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

// The engine's FRotator::Quaternion (degrees)
glm::dquat RotatorToQuat(double pitch, double yaw, double roll) {
    const double half = glm::radians(1.0) * 0.5;
    const double sp = std::sin(pitch * half), cp = std::cos(pitch * half);
    const double sy = std::sin(yaw * half), cy = std::cos(yaw * half);
    const double sr = std::sin(roll * half), cr = std::cos(roll * half);
    return { cr * cp * cy + sr * sp * sy,    // w
             cr * sp * sy - sr * cp * cy,    // x
             -cr * sp * cy - sr * cp * sy,   // y
             cr * cp * sy - sr * sp * cy };  // z
}

// Signed angle from a to b around axis (a and b are projected onto the plane normal to axis)
double SignedAngle(const glm::dvec3& a, const glm::dvec3& b, const glm::dvec3& axis) {
    const glm::dvec3 n = glm::normalize(axis);
    const glm::dvec3 pa = a - n * glm::dot(a, n), pb = b - n * glm::dot(b, n);
    return std::atan2(glm::dot(glm::cross(pa, pb), n), glm::dot(pa, pb));
}

// Turns a subtree rigidly about an axis through its root bone
void RotateSubtree(ue::FTransform* pose, const std::vector<int32_t>& subtree, const glm::dquat& rotation) {
    if (subtree.empty())
        return;
    const glm::dvec3 pivot = ToGlm(pose[subtree.front()].Translation);
    for (const int32_t bone : subtree)
    {
        auto& t = pose[bone];
        t.Translation = ToUe(pivot + rotation * (ToGlm(t.Translation) - pivot));
        t.Rotation = ToUe(glm::normalize(rotation * ToGlm(t.Rotation)));
    }
}

// The bones of the subtree rooted at root, parents before children
std::vector<int32_t> Subtree(const std::vector<int32_t>& parents, int32_t root) {
    std::vector<int32_t> subtree;
    std::vector<bool> inSubtree(parents.size(), false);
    for (int32_t b = root; b >= 0 && b < static_cast<int32_t>(parents.size()); b++)
    {
        inSubtree[b] = b == root || (parents[b] >= 0 && inSubtree[parents[b]]);
        if (inSubtree[b])
            subtree.push_back(b);
    }
    return subtree;
}

// Mesh space (the engine's axes: x forward, y right, z up) to the IK's convention (x right, y up, z forward)
glm::dvec3 ToIK(const glm::dvec3& v) { return { v.y, v.z, v.x }; }

// The head joint (the neck's pivot) behind and below the eyes, in the head's frame: down, back (m at
// reference size; Parger et al. 2018, chasm's head_pivot)
constexpr double HeadPivotDown = 0.046;
constexpr double HeadPivotBack = 0.087;
// Share of the torso's twist each spine joint takes, from the hips up (BaseBody_Spine, Spine1, Spine2)
constexpr double SpineTwistShare[3] = { 0.35, 0.35, 0.3 };
// How far each joint of the chain (Spine, Spine1, Spine2, Neck) may bend to bring the head under the
// headset (degrees), and the share of the remaining bend it takes per pass
constexpr double SpineBendLimitDeg[4] = { 35.0, 30.0, 30.0, 30.0 };
constexpr double SpineBendShare = 0.5;
constexpr int SpineBendPasses = 4;
// The head mapping (ToAnimatedHead) stays in use this long after the last solve
constexpr double HeadMapLifetime = 0.25;

} // namespace

ue::FVector BodyIK::ToAnimatedHead(const ue::FVector& worldPoint)
{
    auto* self = s_instance;
    if (self == nullptr || !self->Enabled || std::chrono::duration<double>(std::chrono::steady_clock::now() - self->m_headSolvedTime).count() > HeadMapLifetime)
        return worldPoint;
    const Transform local = Relative(Transform{ {}, ToGlm(worldPoint), { 1, 1, 1 } }, self->m_headSolved);
    return ToUe(Compose(local, self->m_headAnimated).translation);
}

BodyIK::Transform BodyIK::FollowCamera(const Transform& world) const
{
    return Compose(Relative(world, m_previousCameraOrigin), m_cameraOrigin);
}

void BodyIK::KeepBodyVisible(ue::UObject* pawn)
{
    const auto request = ue::call<ue::EGsrPlayerCameraRequestModelVisibility>(pawn, L"GetCurrentCameraModelVisibility");
    const bool firstPerson = request >= ue::EGsrPlayerCameraRequestModelVisibility::Subjective;
    m_visibleInNormalView.resize(std::size(PlayerManager::BodyMeshNames), false);
    for (size_t i = 0; i < std::size(PlayerManager::BodyMeshNames); i++)
    {
        auto* mesh = ue::get<ue::UObject*>(pawn, PlayerManager::BodyMeshNames[i]);
        if (mesh == nullptr)
            continue;
        const bool visible = ue::get<bool>(mesh, L"bVisible");
        // Only what the game shows in its normal view comes back: what Snake doesn't wear stays hidden
        if (!firstPerson)
            m_visibleInNormalView[i] = visible;
        else if (m_visibleInNormalView[i] && !visible)
            ue::call(mesh, L"SetVisibility", true, false);
    }
}

void BodyIK::Update(HookManager& hookManager, const PlayerManager& playerManager, ue::UObject* hmdActor,
                    const UEVR_Vector3d& cameraPosition, const UEVR_Rotatord& cameraRotation)
{
    if (!Enabled)
        return;
    m_hmd = hmdActor;

    // The camera origin only drives UEVR while the player has control (MGSDeltaPlugin::on_early_calculate_stereo_view_offset)
    const Transform origin{ RotatorToQuat(cameraRotation.pitch, cameraRotation.yaw, cameraRotation.roll),
                            { cameraPosition.x, cameraPosition.y, cameraPosition.z }, { 1, 1, 1 } };
    m_previousCameraOrigin = m_hasCameraOrigin && playerManager.IsPlayerControlled ? m_cameraOrigin : origin;
    m_cameraOrigin = origin;
    m_hasCameraOrigin = playerManager.IsPlayerControlled;

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

    if (playerManager.BodyMeshesVisible && playerManager.IsPlayerControlled)
        KeepBodyVisible(pawn);
}

void BodyIK::UpdateTarget(Arm& arm, ue::UObject* handMesh, ue::UObject* controller)
{
    if (handMesh == arm.target.get() && controller == arm.controller.get())
        return;
    arm.target = handMesh;
    arm.controller = controller;
    arm.handOnController.reset();
    arm.state = {};
    // The controllers move after Snake's animation is evaluated (measured in game), which would leave
    // his hands a frame behind them: his mesh ticks after them instead
    if (controller != nullptr)
        ue::call(m_mesh.get(), L"AddTickPrerequisiteComponent", controller);
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
    m_hmd.reset();
    m_torso.reset();
    m_headForwardLocal.reset();
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
    m_spine = boneIndex(L"BaseBody_Spine");
    m_spine1 = boneIndex(L"BaseBody_Spine1");
    m_spine2 = boneIndex(L"BaseBody_Spine2");
    m_neck = boneIndex(L"BaseBody_Neck");
    m_head = boneIndex(L"BaseBody_Head");
    m_headEnd = boneIndex(L"BaseBody_HeadEnd");
    bool found = m_spine >= 0 && m_spine1 >= 0 && m_spine2 >= 0 && m_neck >= 0 && m_head >= 0 && m_headEnd >= 0;
    if (found)
        m_spineSubtree = Subtree(m_parents, m_spine);
    m_torso.reset();
    m_headForwardLocal.reset();
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
        arm.roll[0] = boneIndex((L"BaseBody_" + side + L"ForeArmRoll1").c_str());
        arm.roll[1] = boneIndex((L"BaseBody_" + side + L"ForeArmRoll2").c_str());
        found = found && arm.upper >= 0 && arm.lower >= 0 && arm.hand >= 0 && arm.index >= 0 && arm.middle >= 0 && arm.pinky >= 0 &&
            arm.roll[0] >= 0 && arm.roll[1] >= 0;
        if (!found)
            continue;

        arm.subtree = Subtree(m_parents, arm.upper);
        for (int r = 0; r < 2; r++)
            arm.rollSubtree[r] = Subtree(m_parents, arm.roll[r]);
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

    const Transform meshToWorld = ToTransform(ue::call<ue::FTransform>(mesh, L"K2_GetComponentToWorld"));
    const Transform animatedHead = ToTransform(pose[m_head]);

    // The VR hands' hand bones, in Snake's mesh space: where Snake's hands go, and how they're turned
    std::optional<Transform> hands[2];
    for (int i = 0; i < 2; i++)
    {
        if (const auto world = TargetHand(m_arms[i]))
            hands[i] = Relative(*world, meshToWorld);
    }

    auto at = [pose](int32_t bone) { return ToGlm(pose[bone].Translation) * CmToM; };
    auto armLength = [&](const Arm& arm) { return glm::distance(at(arm.upper), at(arm.lower)) + glm::distance(at(arm.lower), at(arm.hand)); };
    const double scale = 0.5 * (armLength(m_arms[0]) + armLength(m_arms[1])) / ReferenceArmLength;

    SolveUpperBody(pose, meshToWorld, hands, scale, dt);

    // The upper chest as the spine now has it: up the spine, right across the shoulders
    ik::ChestFrame chest;
    chest.pos = at(m_spine2);
    chest.up = glm::normalize(at(m_neck) - chest.pos);
    const glm::dvec3 across = at(m_arms[1].upper) - at(m_arms[0].upper);
    chest.right = glm::normalize(across - chest.up * glm::dot(across, chest.up));
    chest.forward = glm::cross(chest.right, chest.up);

    for (int i = 0; i < 2; i++)
    {
        if (hands[i])
            SolveArm(m_arms[i], pose, chest, *hands[i], dt);
    }

    m_headAnimated = Compose(animatedHead, meshToWorld);
    m_headSolved = Compose(ToTransform(pose[m_head]), meshToWorld);
    m_headSolvedTime = now;
}

void BodyIK::SolveUpperBody(ue::FTransform* pose, const Transform& meshToWorld, const std::optional<Transform> hands[2], double scale, double dt)
{
    auto* hmdActor = m_hmd.get();
    auto* hmdRoot = hmdActor != nullptr ? ue::get<ue::UObject*>(hmdActor, L"RootComponent") : nullptr;
    if (hmdRoot == nullptr)
    {
        m_torso.reset();
        return;
    }

    // The headset in mesh space (cm): UEVR moves this actor with it. Its x looks forward, z up.
    const Transform hmd = Relative(FollowCamera(ToTransform(ue::call<ue::FTransform>(hmdRoot, L"K2_GetComponentToWorld"))), meshToWorld);
    const glm::dvec3 hmdForward = hmd.rotation * glm::dvec3{ 1, 0, 0 };
    const glm::dvec3 hmdUp = hmd.rotation * glm::dvec3{ 0, 0, 1 };
    const glm::dvec3 eyes = hmd.translation;

    auto at = [pose](int32_t bone) { return ToGlm(pose[bone].Translation); };
    const glm::dvec3 up{ 0, 0, 1 };

    // The chest as the game animated it
    const glm::dvec3 chestUp = glm::normalize(at(m_neck) - at(m_spine2));
    const glm::dvec3 across = at(m_arms[1].upper) - at(m_arms[0].upper);
    const glm::dvec3 chestRight = glm::normalize(across - chestUp * glm::dot(across, chestUp));
    const glm::dvec3 chestForward = glm::cross(chestRight, chestUp);

    // The head bone's own axes: up to the top of the head, and the way the face looks, taken once from
    // the game's pose (the face looks the way the chest faces)
    const glm::dquat animatedHeadRot = ToGlm(pose[m_head].Rotation);
    const glm::dvec3 headUpLocal = glm::normalize(glm::inverse(animatedHeadRot) * (at(m_headEnd) - at(m_head)));
    if (!m_headForwardLocal)
    {
        const glm::dvec3 f = glm::inverse(animatedHeadRot) * chestForward;
        m_headForwardLocal = glm::normalize(f - headUpLocal * glm::dot(f, headUpLocal));
    }
    // The head turns with the headset, and goes where Snake's eyes are at the camera
    const glm::dquat headRot = AlignPair(*m_headForwardLocal, headUpLocal, hmdForward, hmdUp);
    const glm::dvec3 headJoint = eyes - (hmdUp * HeadPivotDown + hmdForward * HeadPivotBack) * scale / CmToM;

    // ---- torso yaw: where the hands and head have the chest face (chasm's torso.rs) ----
    const glm::dvec3 forwardIK = ToIK(hmdForward), upIK = ToIK(hmdUp);
    // Looking straight down, the top of the head still points the way you face
    glm::dvec3 facing = forwardIK - upIK * forwardIK.y;
    facing.y = 0.0;
    const double headYaw = glm::length(facing) > 1e-4 ? ik::YawOf(facing) : (m_torso ? m_torso->Yaw() : 0.0);
    if (!m_torso)
        m_torso.emplace(headYaw);
    std::optional<glm::dvec3> handsIK[2];
    for (int i = 0; i < 2; i++)
    {
        if (hands[i])
            handsIK[i] = ToIK(hands[i]->translation * CmToM);
    }
    const double torsoYaw = m_torso->Update(m_torsoParams, dt, scale, ToIK(eyes * CmToM), ToIK(headJoint * CmToM), headYaw, handsIK);

    const glm::dvec3 chestForwardIK = ToIK(chestForward);
    const double twist = ik::WrapAngle(torsoYaw - std::atan2(chestForwardIK.x, chestForwardIK.z));

    // ---- the chain: BaseBody_Spine, Spine1, Spine2, Neck, Head ----
    const int32_t bones[5] = { m_spine, m_spine1, m_spine2, m_neck, m_head };
    Transform chain[5];
    for (int k = 0; k < 5; k++)
        chain[k] = ToTransform(pose[bones[k]]);
    // Turns the chain from joint k up, rigidly about joint k
    auto rotateFrom = [&chain](int k, const glm::dquat& q) {
        for (int j = k; j < 5; j++)
        {
            if (j > k)
                chain[j].translation = chain[k].translation + q * (chain[j].translation - chain[k].translation);
            chain[j].rotation = glm::normalize(q * chain[j].rotation);
        }
    };

    // The torso's twist, spread up the spine (the hips keep the game's)
    for (int k = 0; k < 3; k++)
        rotateFrom(k, glm::angleAxis(twist * SpineTwistShare[k], up));

    // Bend the spine and neck to bring the head joint under the headset (cyclic coordinate descent,
    // each joint taking a share of what's left, within its limit)
    glm::dquat bent[4] = { { 1, 0, 0, 0 }, { 1, 0, 0, 0 }, { 1, 0, 0, 0 }, { 1, 0, 0, 0 } };
    for (int pass = 0; pass < SpineBendPasses; pass++)
    {
        for (int k = 3; k >= 0; k--)
        {
            const glm::dvec3 current = chain[4].translation - chain[k].translation;
            const glm::dvec3 wanted = headJoint - chain[k].translation;
            if (glm::length(current) < 1.0 || glm::length(wanted) < 1.0)
                continue;
            glm::dquat q = glm::slerp(glm::dquat{ 1, 0, 0, 0 }, glm::rotation(glm::normalize(current), glm::normalize(wanted)), SpineBendShare);
            // Within the joint's limit, counting what it already bent
            const double limit = glm::radians(SpineBendLimitDeg[k]);
            const glm::dquat total = glm::normalize(q * bent[k]);
            const double angle = glm::angle(total);
            if (angle > limit)
            {
                const double already = glm::angle(bent[k]);
                const double qAngle = glm::angle(q);
                const double allowed = std::max(limit - already, 0.0);
                q = qAngle > 1e-6 ? glm::slerp(glm::dquat{ 1, 0, 0, 0 }, q, std::min(allowed / qAngle, 1.0)) : glm::dquat{ 1, 0, 0, 0 };
            }
            bent[k] = glm::normalize(q * bent[k]);
            rotateFrom(k, q);
        }
    }

    chain[4].rotation = headRot;

    BoneOverride overrides[5];
    for (int k = 0; k < 5; k++)
        overrides[k] = { bones[k], chain[k].rotation, chain[k].translation };
    WriteSubtree(pose, m_spineSubtree, overrides, std::size(overrides));
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
    const Transform handMeshWorld = Compose(arm.handOnController.value_or(DefaultHandOnController), FollowCamera(controllerWorld));
    const Transform bone = ToTransform(ue::call<ue::FTransform>(handMesh, L"GetBoneTransform", arm.handName,
        ue::ERelativeTransformSpace::RTS_Component));
    return Compose(bone, handMeshWorld);
}

void BodyIK::SolveArm(Arm& arm, ue::FTransform* pose, const ik::ChestFrame& chest, const Transform& target, double dt)
{
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
    TwistForearm(arm, pose, lowerRot, handRot);
}

void BodyIK::TwistForearm(Arm& arm, ue::FTransform* pose, const glm::dquat& animLowerRot, const glm::dquat& animHandRot)
{
    // The forearm keeps the game's twist and the hand takes the controller's: the roll between them,
    // measured against the game's pose, is spread along the forearm by where each twist bone sits
    // (the radius turning about the ulna), so the skin doesn't wring at the wrist.
    auto at = [pose](int32_t bone) { return ToGlm(pose[bone].Translation); };
    const glm::dquat lowerRot = ToGlm(pose[arm.lower].Rotation);
    const glm::dquat handRot = ToGlm(pose[arm.hand].Rotation);
    const glm::dvec3 forearm = at(arm.hand) - at(arm.lower);
    const double length = glm::length(forearm);
    if (length < 1.0)
        return;
    const glm::dvec3 axis = forearm / length;
    const glm::dvec3 thumbLocal = glm::inverse(handRot) * (at(arm.index) - at(arm.pinky));
    const glm::dvec3 animatedThumb = glm::inverse(animLowerRot) * (animHandRot * thumbLocal);
    const glm::dvec3 solvedThumb = glm::inverse(lowerRot) * (handRot * thumbLocal);
    const double roll = SignedAngle(animatedThumb, solvedThumb, glm::inverse(lowerRot) * axis);
    for (int r = 0; r < 2; r++)
    {
        const double share = std::clamp(glm::dot(at(arm.roll[r]) - at(arm.lower), axis) / length, 0.0, 1.0);
        RotateSubtree(pose, arm.rollSubtree[r], glm::angleAxis(roll * share, axis));
    }
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
