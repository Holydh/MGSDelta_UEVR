#include "BodyIK.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <utility>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "renderlib/imgui/imgui.h"

#include "HookManager.hpp"
#include "Logger.hpp"
#include "PlayerManager.hpp"

namespace {

// Same layout as the engine's TArray
struct RawArray {
    void* data;
    int32_t num;
    int32_t max;
};

// In USkinnedMeshComponent, after ComponentSpaceTransformsArray[2] (engine layout, UE 5.x): the
// indices of the buffer being written and of the one being read, at these offsets in some order
constexpr int32_t BufferIndicesAfterPoseArrays[] = { 0x44, 0x48 };
// A pose buffer matches the engine's bone transform within this distance (cm)
constexpr double PoseMatchTolerance = 0.01;
// Frames the pose buffers are looked for before giving up
constexpr int32_t MaxPoseBufferAttempts = 300;
// Range of the mesh component scanned for the pose buffers (USkeletalMeshComponent is 0xFC0 bytes)
constexpr int32_t ScanBegin = 0x400;
constexpr int32_t ScanEnd = 0xF00;

glm::dvec3 ToGlm(const ue::FVector& v) { return { v.X, v.Y, v.Z }; }
glm::dquat ToGlm(const ue::FQuat& q) { return { q.W, q.X, q.Y, q.Z }; }
ue::FVector ToUe(const glm::dvec3& v) { return { v.x, v.y, v.z }; }
ue::FQuat ToUe(const glm::dquat& q) { return { q.x, q.y, q.z, q.w }; }

bool IsPlausibleTransform(const ue::FTransform& t) {
    const double quatLength = glm::length(ToGlm(t.Rotation));
    return std::abs(quatLength - 1.0) < 0.01 && t.Scale3D.X > 0.0 && t.Scale3D.X < 100.0;
}

// The pose is in cm, the solver works in metres
constexpr double CmToM = 0.01;
// Arm length of the reference body the solver's constants are tuned for (m)
constexpr double ReferenceArmLength = 0.6;
// The solver's filters see at most three frames' time at once, so one hitch can't fling them
constexpr double MaxDeltaTime = 3.0 / 90.0;

using Transform = BodyIK::Transform;

Transform ToTransform(const ue::FTransform& t) { return { ToGlm(t.Rotation), ToGlm(t.Translation), ToGlm(t.Scale3D) }; }

// child relative to parent, then parent: the engine's child * parent
Transform Compose(const Transform& child, const Transform& parent) {
    return { glm::normalize(parent.rotation * child.rotation), parent.translation + parent.rotation * (parent.scale * child.translation),
             parent.scale * child.scale };
}

// transform, relative to parent
Transform Relative(const Transform& transform, const Transform& parent) {
    const glm::dquat inverse = glm::inverse(parent.rotation);
    return { glm::normalize(inverse * transform.rotation), (inverse * (transform.translation - parent.translation)) / parent.scale,
             transform.scale / parent.scale };
}

// The VR hand mesh relative to its controller when nothing moved it: what WeaponManager::DetachHandFromWeapon
// restores (no offset, -90° yaw)
const Transform DefaultHandOnController{ glm::angleAxis(glm::radians(-90.0), glm::dvec3{ 0, 0, 1 }), {}, { 1, 1, 1 } };

// The rotation taking the pair of directions (a1, b1) onto (a2, b2): a1 exactly onto a2, b1 as close
// to b2 as it can go
glm::dquat AlignPair(const glm::dvec3& a1, const glm::dvec3& b1, const glm::dvec3& a2, const glm::dvec3& b2) {
    auto basis = [](const glm::dvec3& a, const glm::dvec3& b) {
        const glm::dvec3 x = glm::normalize(a);
        const glm::dvec3 y = glm::normalize(b - x * glm::dot(b, x));
        return glm::dmat3(x, y, glm::cross(x, y));
    };
    return glm::normalize(glm::quat_cast(basis(a2, b2) * glm::transpose(basis(a1, b1))));
}

} // namespace

void BodyIK::Update(HookManager& hookManager, const PlayerManager& playerManager)
{
    if (!Enabled)
        return;

    auto pawn = uevr::API::get()->get_local_pawn(0);
    // ACharacter::Mesh. The game's name table spells this property in lowercase.
    auto mesh = ue::get<ue::UObject*>(pawn, L"mesh");
    if (mesh == nullptr)
        return;

    if (mesh != m_mesh.get() || ue::call<int32_t>(mesh, L"GetNumBones") != m_numBones)
    {
        m_mesh.reset();
        m_readIndexOffset = -1;
        m_poseBufferAttempts = 0;
        if (mesh == m_failedMesh.get())
            return;
        if (!Bind(mesh))
        {
            m_failedMesh = mesh;
            return;
        }
        m_mesh = mesh;
    }

    if (m_readIndexOffset < 0)
    {
        const auto found = FindPoseBuffers(mesh);
        if (found == PoseBuffers::Retry && ++m_poseBufferAttempts < MaxPoseBufferAttempts)
            return;
        if (found != PoseBuffers::Found)
        {
            uevr::API::get()->log_error("[MGSDelta] BodyIK: Snake's pose buffers weren't found, full body IK disabled for this mesh "
                "(last check: distances %.4f / %.4f cm, indices %d / %d)", m_lastMismatch.distances[0], m_lastMismatch.distances[1],
                m_lastMismatch.indices[0], m_lastMismatch.indices[1]);
            m_failedMesh = mesh;
            m_mesh.reset();
            return;
        }
        Logger::DebugPrint("BodyIK: pose buffers at 0x%X, read index at 0x%X", m_poseArraysOffset, m_readIndexOffset);
    }

    m_animInstance = ue::get<ue::UObject*>(mesh, L"AnimScriptInstance");
    if (!m_hookInstalled && m_animInstance.get())
    {
        // Snake's anim blueprint implements this event: the engine calls it on the game thread once the
        // frame's pose is evaluated, just before it's published to the renderer and the follower meshes.
        // Installed once, HookManager applies it again if the class gets reloaded.
        m_hookInstalled = true;
        const auto className = m_animInstance->get_class()->get_full_name();
        const bool hooked = hookManager.hook_bp_fn(className, L"BlueprintPostEvaluateAnimation", nullptr,
            (UEVR_UFunction_NativePostFn)&BodyIK::OnPostEvaluateAnimation, true);
        uevr::API::get()->log_info("[MGSDelta] BodyIK: post evaluate hook on %s %s", ue::narrow(className).c_str(), hooked ? "installed" : "failed");
    }

    UpdateTarget(m_arms[0], playerManager.LeftHandMesh, playerManager.LeftController);
    UpdateTarget(m_arms[1], playerManager.RightHandMesh, playerManager.RightController);
}

void BodyIK::UpdateTarget(Arm& arm, ue::UObject* handMesh, ue::UObject* controller)
{
    if (handMesh == arm.target.get() && controller == arm.controller.get())
        return;
    arm.target = handMesh;
    arm.controller = controller;
    arm.handOnController.reset();
    arm.state = {};
    // GetBoneIndex returns -1 (INDEX_NONE) for a bone the mesh doesn't have
    arm.targetHasHand = handMesh != nullptr && ue::call<int32_t>(handMesh, L"GetBoneIndex", arm.handName) >= 0;
    if (handMesh != nullptr && !arm.targetHasHand)
        uevr::API::get()->log_error("[MGSDelta] BodyIK: the VR hand mesh has no %s bone", arm.handName.ToString().c_str());
}

void BodyIK::Cleanup()
{
    m_mesh.reset();
    m_failedMesh.reset();
    m_animInstance.reset();
    for (auto& arm : m_arms)
    {
        arm.target.reset();
        arm.targetHasHand = false;
        arm.controller.reset();
        arm.handOnController.reset();
        arm.state = {};
        arm.hingeLocal.reset();
    }
}

bool BodyIK::Bind(ue::UObject* mesh)
{
    m_numBones = ue::call<int32_t>(mesh, L"GetNumBones");
    if (m_numBones <= 0)
        return false;

    std::vector<ue::FName> names(m_numBones);
    std::map<std::pair<int32_t, int32_t>, int32_t> indexByName;
    for (int32_t i = 0; i < m_numBones; i++)
    {
        names[i] = ue::call<ue::FName>(mesh, L"GetBoneName", i);
        indexByName[{ names[i].ComparisonIndex, names[i].Number }] = i;
    }

    // The pose is updated from the root down, which needs every parent before its children
    m_parents.assign(m_numBones, -1);
    for (int32_t i = 1; i < m_numBones; i++)
    {
        const auto parent = ue::call<ue::FName>(mesh, L"GetParentBone", names[i]);
        const auto it = indexByName.find({ parent.ComparisonIndex, parent.Number });
        if (it == indexByName.end() || it->second >= i)
        {
            uevr::API::get()->log_error("[MGSDelta] BodyIK: unexpected parent for bone %d (%s)", i, names[i].ToString().c_str());
            return false;
        }
        m_parents[i] = it->second;
    }

    auto boneIndex = [&](const wchar_t* name) {
        const ue::FName fname{ name };
        const auto it = indexByName.find({ fname.ComparisonIndex, fname.Number });
        if (it == indexByName.end())
            uevr::API::get()->log_error("[MGSDelta] BodyIK: bone %ls not found", name);
        return it != indexByName.end() ? it->second : -1;
    };
    m_spine2 = boneIndex(L"BaseBody_Spine2");
    m_neck = boneIndex(L"BaseBody_Neck");
    bool found = m_spine2 >= 0 && m_neck >= 0;
    for (int i = 0; i < 2; i++)
    {
        auto& arm = m_arms[i];
        const std::wstring side = i == 0 ? L"Left" : L"Right";
        arm.side = i == 0 ? ik::Side::Left : ik::Side::Right;
        arm.upper = boneIndex((L"BaseBody_" + side + L"Arm").c_str());
        arm.lower = boneIndex((L"BaseBody_" + side + L"ForeArm").c_str());
        arm.hand = boneIndex((L"BaseBody_" + side + L"Hand").c_str());
        arm.index = boneIndex((L"BaseBody_" + side + L"HandIndex1").c_str());
        arm.middle = boneIndex((L"BaseBody_" + side + L"HandMiddle1").c_str());
        arm.pinky = boneIndex((L"BaseBody_" + side + L"HandPinky1").c_str());
        arm.handName = arm.hand >= 0 ? names[arm.hand] : ue::FName{};
        found = found && arm.upper >= 0 && arm.lower >= 0 && arm.hand >= 0 && arm.index >= 0 && arm.middle >= 0 && arm.pinky >= 0;
        if (!found)
            continue;

        std::vector<bool> inSubtree(m_numBones, false);
        arm.subtree.clear();
        for (int32_t b = arm.upper; b < m_numBones; b++)
        {
            inSubtree[b] = b == arm.upper || inSubtree[m_parents[b]];
            if (inSubtree[b])
                arm.subtree.push_back(b);
        }
        // Targets and filters belong to the previous mesh
        arm.target.reset();
        arm.targetHasHand = false;
        arm.controller.reset();
        arm.handOnController.reset();
        arm.state = {};
        arm.hingeLocal.reset();
    }
    if (!found)
        return false;

    m_scratch.resize(m_numBones);
    Logger::DebugPrint("BodyIK: bound to %d bones", m_numBones);
    return true;
}

BodyIK::PoseBuffers BodyIK::FindPoseBuffers(ue::UObject* mesh)
{
    // ComponentSpaceTransformsArray[2]: two consecutive arrays with one transform per bone
    const auto base = reinterpret_cast<const uint8_t*>(mesh);
    m_poseArraysOffset = -1;
    for (int32_t offset = ScanBegin; offset < ScanEnd; offset += 8)
    {
        const auto arrays = reinterpret_cast<const RawArray*>(base + offset);
        bool match = true;
        for (int i = 0; i < 2 && match; i++)
        {
            match = arrays[i].data != nullptr && arrays[i].num == m_numBones && arrays[i].max >= m_numBones &&
                IsPlausibleTransform(*static_cast<const ue::FTransform*>(arrays[i].data));
        }
        if (match)
        {
            m_poseArraysOffset = offset;
            break;
        }
    }
    if (m_poseArraysOffset < 0)
    {
        uevr::API::get()->log_error("[MGSDelta] BodyIK: pose buffers not found in the mesh component");
        return PoseBuffers::NotFound;
    }

    // The buffer being read holds what the engine's GetBoneTransform returns. Snake is never quite
    // still (breathing), so the other buffer, a frame older, differs.
    const auto arrays = reinterpret_cast<const RawArray*>(base + m_poseArraysOffset);
    const int32_t hand = m_arms[1].hand;
    const auto expected = ToGlm(ue::call<ue::FTransform>(mesh, L"GetBoneTransform", m_arms[1].handName,
        ue::ERelativeTransformSpace::RTS_Component).Translation);
    double distances[2];
    for (int i = 0; i < 2; i++)
        distances[i] = glm::length(ToGlm(static_cast<const ue::FTransform*>(arrays[i].data)[hand].Translation) - expected);
    const int read = distances[0] <= distances[1] ? 0 : 1;
    if (distances[1 - read] <= PoseMatchTolerance)
        return PoseBuffers::Retry; // both buffers hold the same pose: can't tell them apart yet

    const auto indexAt = [base, this](int32_t offset) { return *reinterpret_cast<const int32_t*>(base + m_poseArraysOffset + offset); };
    const int32_t first = indexAt(BufferIndicesAfterPoseArrays[0]);
    const int32_t second = indexAt(BufferIndicesAfterPoseArrays[1]);
    if (distances[read] <= PoseMatchTolerance && first + second == 1 && (first == 0 || first == 1))
    {
        m_readIndexOffset = m_poseArraysOffset + BufferIndicesAfterPoseArrays[first == read ? 0 : 1];
        return PoseBuffers::Found;
    }

    m_lastMismatch = { distances[0], distances[1], first, second };
    return PoseBuffers::Retry;
}

int32_t BodyIK::ReadIndex() const
{
    auto base = reinterpret_cast<const uint8_t*>(m_mesh.get());
    if (base == nullptr || m_readIndexOffset < 0)
        return -1;
    const int32_t read = *reinterpret_cast<const int32_t*>(base + m_readIndexOffset);
    return read == 0 || read == 1 ? read : -1;
}

ue::FTransform* BodyIK::PoseBuffer(int32_t index) const
{
    auto base = reinterpret_cast<const uint8_t*>(m_mesh.get());
    if (base == nullptr || m_poseArraysOffset < 0 || (index != 0 && index != 1))
        return nullptr;
    const auto& array = reinterpret_cast<const RawArray*>(base + m_poseArraysOffset)[index];
    return array.num == m_numBones ? static_cast<ue::FTransform*>(array.data) : nullptr;
}

bool BodyIK::OnPostEvaluateAnimation(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result)
{
    auto* self = s_instance;
    if (self != nullptr && self->Enabled && obj != nullptr && obj == self->m_animInstance.get())
        self->ApplyPose();
    return true;
}

void BodyIK::ApplyPose()
{
    // The event runs just before the pose buffers flip (checked in game): the new pose is in the
    // buffer that isn't the current one yet, the current one still holds last frame's
    const int32_t read = ReadIndex();
    auto* pose = read >= 0 ? PoseBuffer(1 - read) : nullptr;
    auto* mesh = m_mesh.get();
    if (pose == nullptr || mesh == nullptr)
        return;

    const auto now = std::chrono::steady_clock::now();
    const double dt = m_lastSolve == std::chrono::steady_clock::time_point{} ? 0.0
        : std::clamp(std::chrono::duration<double>(now - m_lastSolve).count(), 0.0, MaxDeltaTime);
    m_lastSolve = now;

    auto at = [pose](int32_t bone) { return ToGlm(pose[bone].Translation) * CmToM; };
    // The upper chest, from Snake's own pose: up the spine, right across the shoulders
    ik::ChestFrame chest;
    chest.pos = at(m_spine2);
    chest.up = glm::normalize(at(m_neck) - chest.pos);
    const glm::dvec3 across = at(m_arms[1].upper) - at(m_arms[0].upper);
    chest.right = glm::normalize(across - chest.up * glm::dot(across, chest.up));
    chest.forward = glm::cross(chest.right, chest.up);

    const auto meshToWorld = ue::call<ue::FTransform>(mesh, L"K2_GetComponentToWorld");
    for (auto& arm : m_arms)
        SolveArm(arm, pose, chest, meshToWorld, dt);
}

std::optional<BodyIK::Transform> BodyIK::TargetHand(Arm& arm) const
{
    auto* handMesh = arm.target.get();
    auto* controller = arm.controller.get();
    if (handMesh == nullptr || controller == nullptr || !arm.targetHasHand)
        return std::nullopt;

    const Transform controllerWorld = ToTransform(ue::call<ue::FTransform>(controller, L"K2_GetComponentToWorld"));
    if (ue::call<ue::UObject*>(handMesh, L"GetAttachParent") == controller)
        arm.handOnController = Relative(ToTransform(ue::call<ue::FTransform>(handMesh, L"K2_GetComponentToWorld")), controllerWorld);
    const Transform handMeshWorld = Compose(arm.handOnController.value_or(DefaultHandOnController), controllerWorld);
    const Transform bone = ToTransform(ue::call<ue::FTransform>(handMesh, L"GetBoneTransform", arm.handName,
        ue::ERelativeTransformSpace::RTS_Component));
    return Compose(bone, handMeshWorld);
}

void BodyIK::SolveArm(Arm& arm, ue::FTransform* pose, const ik::ChestFrame& chest, const ue::FTransform& meshToWorld, double dt)
{
    // The VR hand's hand bone, in Snake's mesh space: where Snake's hand goes, and how it's turned
    const auto targetWorld = TargetHand(arm);
    if (!targetWorld)
        return;
    const Transform target = Relative(*targetWorld, ToTransform(meshToWorld));
    const glm::dvec3 targetPos = target.translation;
    const glm::dquat targetRot = target.rotation;

    auto at = [pose](int32_t bone) { return ToGlm(pose[bone].Translation) * CmToM; };
    const glm::dvec3 shoulder = at(arm.upper), elbow = at(arm.lower), wrist = at(arm.hand);
    const glm::dquat upperRot = ToGlm(pose[arm.upper].Rotation);
    const glm::dquat lowerRot = ToGlm(pose[arm.lower].Rotation);
    const glm::dquat handRot = ToGlm(pose[arm.hand].Rotation);

    ik::ArmSkeleton skel;
    skel.upperArm = glm::distance(shoulder, elbow);
    skel.lowerArm = glm::distance(elbow, wrist);
    skel.scale = skel.ArmLength() / ReferenceArmLength;
    skel.shoulderUp = 0.027 * skel.scale;

    // The hand's axes (towards the fingers, towards the thumb side), carried from Snake's hand bone onto the target
    const glm::dvec3 fingersLocal = glm::inverse(handRot) * (at(arm.middle) - wrist);
    const glm::dvec3 thumbLocal = glm::inverse(handRot) * (at(arm.index) - at(arm.pinky));
    ik::HandTarget hand;
    hand.wrist = targetPos * CmToM;
    hand.fingers = glm::normalize(targetRot * fingersLocal);
    const glm::dvec3 thumb = targetRot * thumbLocal;
    hand.thumb = glm::normalize(thumb - hand.fingers * glm::dot(thumb, hand.fingers));

    const auto solved = ik::SolveArm(m_armParams, skel, dt, arm.state, arm.side, chest, shoulder, hand);
    if (!solved)
        return; // tracking lost: the arm keeps the game's animation

    // The elbow hinge in the game's pose. A straight arm has none: the last one seen is kept, in the
    // upper arm's frame.
    const glm::dvec3 upperDir = glm::normalize(elbow - shoulder);
    const glm::dvec3 lowerDir = glm::normalize(wrist - elbow);
    const glm::dvec3 bend = glm::cross(upperDir, lowerDir);
    if (glm::length(bend) > 0.17) // bent more than 10°
        arm.hingeLocal = glm::inverse(upperRot) * glm::normalize(bend);
    const glm::dvec3 hinge = arm.hingeLocal ? upperRot * *arm.hingeLocal : glm::dvec3{};
    const glm::dvec3 solvedHinge = glm::normalize(glm::cross(solved->upperDir, solved->fold));

    // Each bone turns onto its solved direction, keeping its twist about it and the elbow's hinge
    auto turn = [&](const glm::dvec3& from, const glm::dvec3& to) {
        return arm.hingeLocal ? AlignPair(from, hinge, to, solvedHinge) : glm::rotation(from, to);
    };
    const BoneOverride overrides[] = {
        { arm.upper, glm::normalize(turn(upperDir, solved->upperDir) * upperRot), ToGlm(pose[arm.upper].Translation) },
        { arm.lower, glm::normalize(turn(lowerDir, solved->forearmDir) * lowerRot), solved->elbow / CmToM },
        { arm.hand, targetRot, solved->wrist / CmToM },
    };
    WriteSubtree(pose, arm.subtree, overrides, std::size(overrides));
}

void BodyIK::WriteSubtree(ue::FTransform* pose, const std::vector<int32_t>& subtree, const BoneOverride* overrides, size_t count)
{
    for (const int32_t bone : subtree)
        m_scratch[bone] = pose[bone];

    for (const int32_t bone : subtree)
    {
        auto& t = pose[bone];
        const BoneOverride* set = nullptr;
        for (size_t i = 0; i < count && set == nullptr; i++)
            set = overrides[i].bone == bone ? &overrides[i] : nullptr;
        if (set != nullptr)
        {
            t.Rotation = ToUe(set->rotation);
            t.Translation = ToUe(set->translation);
            continue;
        }

        // Keep the bone's transform relative to its parent, as the game posed it
        const int32_t parent = m_parents[bone];
        const auto& oldParent = m_scratch[parent];
        const auto& newParent = pose[parent];
        const glm::dquat oldParentRot = ToGlm(oldParent.Rotation);
        const glm::dvec3 parentScale = ToGlm(oldParent.Scale3D);
        const glm::dquat localRot = glm::inverse(oldParentRot) * ToGlm(m_scratch[bone].Rotation);
        const glm::dvec3 localPos = (glm::inverse(oldParentRot) * (ToGlm(m_scratch[bone].Translation) - ToGlm(oldParent.Translation))) / parentScale;
        const glm::dquat newParentRot = ToGlm(newParent.Rotation);
        t.Rotation = ToUe(glm::normalize(newParentRot * localRot));
        t.Translation = ToUe(ToGlm(newParent.Translation) + newParentRot * (localPos * parentScale));
    }
}

void BodyIK::DrawMenu()
{
    auto* self = s_instance;
    if (self == nullptr || !ImGui::CollapsingHeader("Full body IK (experimental)"))
        return;

    ImGui::Checkbox("Enable", &self->Enabled);
    ImGui::Text("Mesh bound: %s, bones: %d, pose buffers: %s", self->m_mesh.get() ? "yes" : "no", self->m_numBones,
        self->m_readIndexOffset >= 0 ? "found" : "not found");
    ImGui::Text("VR hands: left %s, right %s", self->m_arms[0].targetHasHand ? "found" : "not found",
        self->m_arms[1].targetHasHand ? "found" : "not found");
}
