#include "BodyIK.hpp"

#include <algorithm>
#include <bit>
#include <cwctype>
#include <iterator>
#include <map>
#include <numbers>
#include <span>
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
glm::dvec3 FromIK(const glm::dvec3& v) { return { v.z, v.x, v.y }; }

// The head joint (the neck's pivot) behind and below the eyes, in the head's frame: down, back (m at
// reference size; Parger et al. 2018, chasm's head_pivot)
constexpr double HeadPivotDown = 0.046;
constexpr double HeadPivotBack = 0.087;
// Smoothing of the eyes' speed over the ground (s), and the speed above which the body counts as
// moving (m/s, VRIK's move threshold)
constexpr double VelocityTau = 0.1;
constexpr double MovingSpeed = 0.3;
// Standing straight, the legs are a sliver short of straight, so a knee keeps a few degrees of bend (chasm, FRIK)
constexpr double LegSlack = 0.998;
// Crawling, the trunk lies flat behind the head (spine pitch, radians), and gets there this slowly (s)
constexpr double PronePitch = 90.0 * std::numbers::pi / 180.0;
constexpr double ProneTau = 0.3;
// The centre of mass is this share of the way from the hips to the head joint (chasm, against motion capture)
constexpr double ComUpTheSpine = 0.3;
// The body this far from its feet went somewhere without walking (a teleport, a cutscene): they start
// again under it (m)
constexpr double FeetResetDistance = 1.5;
// The head mapping (ToAnimatedHead) stays in use this long after the last solve
constexpr double HeadMapLifetime = 0.25;

// The meshes registered in Snake's model parts controller (UGsrModelPartsController, not reflected): a
// TArray of these. Each frame the game sets their visibility from the camera's request, by part.
// (Its UnregisterMesh_BP is an empty stub in the shipping game.)
constexpr int32_t RegisteredPartsOffset = 0xA8;
struct RegisteredPart {
    ue::UObject* mesh;
    uint32_t partBit; // 1 << EGsrPlayerModelPart
};
// A part bit none of the game's visibility updates selects: a mesh registered with it is left alone
constexpr uint32_t TakenPartBit = 1u << 31;

std::span<RegisteredPart> RegisteredParts(ue::UObject* controller)
{
    if (controller == nullptr)
        return {};
    const auto& array = *reinterpret_cast<const RawArray*>(reinterpret_cast<uint8_t*>(controller) + RegisteredPartsOffset);
    return { static_cast<RegisteredPart*>(array.data), static_cast<size_t>(std::max(array.num, 0)) };
}

bool IsHeadPart(ue::EGsrPlayerModelPart part)
{
    using enum ue::EGsrPlayerModelPart;
    return part == Head || part == Bandana || part == Eyepatch || part == Glass || part == Hat;
}

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

void BodyIK::KeepBodyVisible(ue::UObject* pawn, bool bodyShown)
{
    const bool firstPerson = bodyShown && ue::call<ue::EGsrPlayerCameraRequestModelVisibility>(pawn, L"GetCurrentCameraModelVisibility")
        >= ue::EGsrPlayerCameraRequestModelVisibility::Subjective;
    if (!firstPerson)
    {
        ReleaseBodyParts();
        m_bodyParts.clear();
        m_partsController = bodyShown ? ue::get<ue::UObject*>(pawn, L"ModelPartsController") : nullptr;
        // The body's parts as the game shows them in its normal view
        for (const auto& entry : RegisteredParts(m_partsController.get()))
        {
            if (entry.mesh != nullptr && !IsHeadPart(static_cast<ue::EGsrPlayerModelPart>(std::countr_zero(entry.partBit))))
                m_bodyParts.push_back({ entry.mesh, entry.partBit, ue::get<bool>(entry.mesh, L"bVisible") });
        }
        return;
    }
    if (m_bodyPartsTaken)
        return;
    // The game would hide them every frame: they're taken from it until the normal view is back
    m_bodyPartsTaken = true;
    for (auto& entry : RegisteredParts(m_partsController.get()))
    {
        for (const auto& bodyPart : m_bodyParts)
        {
            if (entry.mesh == nullptr || entry.mesh != bodyPart.mesh.get())
                continue;
            entry.partBit = TakenPartBit;
            ue::call(entry.mesh, L"SetVisibility", bodyPart.visible, false);
        }
    }
}

void BodyIK::ReleaseBodyParts()
{
    if (!m_bodyPartsTaken)
        return;
    m_bodyPartsTaken = false;
    for (auto& entry : RegisteredParts(m_partsController.get()))
    {
        for (const auto& bodyPart : m_bodyParts)
        {
            if (entry.partBit == TakenPartBit && entry.mesh == bodyPart.mesh.get())
                entry.partBit = bodyPart.partBit;
        }
    }
}

void BodyIK::Update(HookManager& hookManager, const PlayerManager& playerManager, ue::UObject* hmdActor,
                    const UEVR_Vector3d& cameraPosition, const UEVR_Rotatord& cameraRotation)
{
    if (!Enabled)
    {
        ReleaseBodyParts();
        return;
    }
    m_hmd = hmdActor;
    m_crawling = playerManager.CurrentStance == ue::EGsrPlayerStance::Crawl;
    m_playerControlled = playerManager.IsPlayerControlled;

    // The camera origin only drives UEVR while the player has control (MGSDeltaPlugin::on_early_calculate_stereo_view_offset)
    const Transform origin{ RotatorToQuat(cameraRotation.pitch, cameraRotation.yaw, cameraRotation.roll),
                            { cameraPosition.x, cameraPosition.y, cameraPosition.z }, { 1, 1, 1 } };
    m_previousCameraOrigin = m_hasCameraOrigin && playerManager.IsPlayerControlled ? m_cameraOrigin : origin;
    m_cameraOrigin = origin;
    m_hasCameraOrigin = playerManager.IsPlayerControlled;

    auto pawn = uevr::API::get()->get_local_pawn(0);
    KeepBodyVisible(pawn, playerManager.BodyMeshesVisible && playerManager.IsPlayerControlled);
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
    if (!m_eyesSearched)
        FindEyes(pawn);
}

void BodyIK::FindEyes(ue::UObject* pawn)
{
    // The body's skeleton has no face: the eyes are in the facial mesh, which shares the head bone. Its
    // own pose is read (component space), so it doesn't matter whether the hidden face animates.
    m_eyesSearched = true;
    auto* face = ue::get<ue::UObject*>(pawn, L"SkeletalMesh_Head");
    if (face == nullptr || ue::call<int32_t>(face, L"GetBoneIndex", m_headName) < 0)
    {
        uevr::API::get()->log_error("[MGSDelta] BodyIK: no head bone in Snake's facial mesh, the head pivot is estimated");
        return;
    }
    auto boneInFace = [face](const ue::FName& name) {
        return ToTransform(ue::call<ue::FTransform>(face, L"GetBoneTransform", name, ue::ERelativeTransformSpace::RTS_Component));
    };
    const Transform head = boneInFace(m_headName);
    glm::dvec3 sum{};
    int count = 0;
    const int32_t numBones = ue::call<int32_t>(face, L"GetNumBones");
    for (int32_t i = 0; i < numBones; i++)
    {
        const auto name = ue::call<ue::FName>(face, L"GetBoneName", i);
        std::wstring lower = name.ToWString();
        std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
        if (lower.find(L"eye") == std::wstring::npos || lower.find(L"brow") != std::wstring::npos)
            continue;
        const glm::dvec3 local = glm::inverse(head.rotation) * (boneInFace(name).translation - head.translation) / head.scale;
        if (glm::length(local) > 25.0) // e.g. a look-at target, not part of the face
            continue;
        sum += local;
        count++;
    }
    if (count == 0)
    {
        uevr::API::get()->log_error("[MGSDelta] BodyIK: no eye bones in Snake's facial mesh, the head pivot is estimated");
        return;
    }
    m_eyesLocal = sum / static_cast<double>(count);
    uevr::API::get()->log_info("[MGSDelta] BodyIK: Snake's eyes from %d facial bones, at %.1f, %.1f, %.1f cm in the head bone's frame (%.1f cm away)",
        count, m_eyesLocal->x, m_eyesLocal->y, m_eyesLocal->z, glm::length(*m_eyesLocal));
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
    ReleaseBodyParts();
    m_bodyParts.clear();
    m_partsController.reset();
    m_mesh.reset();
    m_failedMesh.reset();
    m_animInstance.reset();
    m_hmd.reset();
    m_torso.reset();
    m_root.reset();
    m_lastEyes.reset();
    m_footing.reset();
    m_lastCom.reset();
    m_hipDrop = 0.0;
    m_segmentAxes.reset();
    m_headForwardLocal.reset();
    m_eyesLocal.reset();
    m_eyesSearched = false;
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
    m_hips = boneIndex(L"BaseBody_Hips");
    m_spine = boneIndex(L"BaseBody_Spine");
    m_spine1 = boneIndex(L"BaseBody_Spine1");
    m_spine2 = boneIndex(L"BaseBody_Spine2");
    m_neck = boneIndex(L"BaseBody_Neck");
    m_head = boneIndex(L"BaseBody_Head");
    m_headEnd = boneIndex(L"BaseBody_HeadEnd");
    bool found = m_hips >= 0 && m_spine >= 0 && m_spine1 >= 0 && m_spine2 >= 0 && m_neck >= 0 && m_head >= 0 && m_headEnd >= 0;
    for (int i = 0; i < 2; i++)
    {
        const std::wstring side = i == 0 ? L"Left" : L"Right";
        m_upLegs[i] = boneIndex((L"BaseBody_" + side + L"UpLeg").c_str());
        m_legs[i] = boneIndex((L"BaseBody_" + side + L"Leg").c_str());
        m_feet[i] = boneIndex((L"BaseBody_" + side + L"Foot").c_str());
        m_toes[i] = boneIndex((L"BaseBody_" + side + L"FootFinger1").c_str());
        found = found && m_upLegs[i] >= 0 && m_legs[i] >= 0 && m_feet[i] >= 0 && m_toes[i] >= 0;
    }
    if (found)
    {
        m_hipsSubtree = Subtree(m_parents, m_hips);
        m_headName = names[m_head];
    }
    m_torso.reset();
    m_root.reset();
    m_lastEyes.reset();
    m_footing.reset();
    m_lastCom.reset();
    m_hipDrop = 0.0;
    m_segmentAxes.reset();
    m_headForwardLocal.reset();
    m_eyesLocal.reset();
    m_eyesSearched = false;
    for (int i = 0; i < 2; i++)
    {
        auto& arm = m_arms[i];
        const std::wstring side = i == 0 ? L"Left" : L"Right";
        arm.side = i == 0 ? ik::Side::Left : ik::Side::Right;
        arm.clavicle = boneIndex((L"BaseBody_" + side + L"Shoulder").c_str());
        arm.upper = boneIndex((L"BaseBody_" + side + L"Arm").c_str());
        arm.lower = boneIndex((L"BaseBody_" + side + L"ForeArm").c_str());
        arm.hand = boneIndex((L"BaseBody_" + side + L"Hand").c_str());
        arm.index = boneIndex((L"BaseBody_" + side + L"HandIndex1").c_str());
        arm.middle = boneIndex((L"BaseBody_" + side + L"HandMiddle1").c_str());
        arm.pinky = boneIndex((L"BaseBody_" + side + L"HandPinky1").c_str());
        arm.handName = arm.hand >= 0 ? names[arm.hand] : ue::FName{};
        arm.roll[0] = boneIndex((L"BaseBody_" + side + L"ForeArmRoll1").c_str());
        arm.roll[1] = boneIndex((L"BaseBody_" + side + L"ForeArmRoll2").c_str());
        found = found && arm.clavicle >= 0 && arm.upper >= 0 && arm.lower >= 0 && arm.hand >= 0 && arm.index >= 0 && arm.middle >= 0 &&
            arm.pinky >= 0 && arm.roll[0] >= 0 && arm.roll[1] >= 0;
        if (!found)
            continue;

        arm.clavicleSubtree = Subtree(m_parents, arm.clavicle);
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

    // The skeleton's reference pose, in component space: Snake standing straight
    m_refPose.resize(m_numBones);
    for (int32_t i = 0; i < m_numBones; i++)
    {
        const Transform local = ToTransform(ue::call<ue::FTransform>(mesh, L"GetRefPoseTransform", i));
        m_refPose[i] = m_parents[i] < 0 ? local : Compose(local, m_refPose[m_parents[i]]);
    }
    // The chain bones' own axes in it: up to the next joint, and the body's right (across the hip
    // joints for the hips, across the shoulders above them)
    auto refAt = [this](int32_t bone) { return m_refPose[bone].translation; };
    const int32_t chain[6] = { m_hips, m_spine, m_spine1, m_spine2, m_neck, m_head };
    const glm::dvec3 hipsRight = refAt(m_upLegs[1]) - refAt(m_upLegs[0]);
    const glm::dvec3 shouldersRight = refAt(m_arms[1].upper) - refAt(m_arms[0].upper);
    std::array<SegmentAxes, 5> axes;
    for (int k = 0; k < 5; k++)
    {
        const glm::dquat inverse = glm::inverse(m_refPose[chain[k]].rotation);
        axes[k] = { glm::normalize(inverse * (refAt(chain[k + 1]) - refAt(chain[k]))), glm::normalize(inverse * (k == 0 ? hipsRight : shouldersRight)) };
    }
    m_segmentAxes = axes;
    // The head bone's: up to the top of the head, and the way the face looks (the way the chest faces)
    const glm::dquat headInverse = glm::inverse(m_refPose[m_head].rotation);
    m_headUpLocal = glm::normalize(headInverse * (refAt(m_headEnd) - refAt(m_head)));
    const glm::dvec3 chestUp = glm::normalize(refAt(m_neck) - refAt(m_spine2));
    const glm::dvec3 chestForward = glm::cross(glm::normalize(shouldersRight - chestUp * glm::dot(shouldersRight, chestUp)), chestUp);
    const glm::dvec3 f = headInverse * chestForward;
    m_headForwardLocal = glm::normalize(f - m_headUpLocal * glm::dot(f, m_headUpLocal));

    // The elbows' hinge, in the upper arms' frames: the forearm folds forward (the reference arm may be
    // straight, which shows no hinge)
    for (auto& arm : m_arms)
    {
        const glm::dvec3 upperDir = glm::normalize(refAt(arm.lower) - refAt(arm.upper));
        arm.hingeLocal = glm::normalize(glm::inverse(m_refPose[arm.upper].rotation) * glm::cross(upperDir, chestForward));
    }

    // Snake's lengths, standing on the floor (the mesh's origin)
    for (int k = 0; k < 4; k++)
        m_spineSkel.spine[k] = glm::distance(refAt(chain[k]), refAt(chain[k + 1])) * CmToM;
    m_spineSkel.neck = glm::distance(refAt(m_neck), refAt(m_head)) * CmToM;
    const glm::dvec3 forward = glm::normalize(glm::dvec3{ chestForward.x, chestForward.y, 0.0 });
    const glm::dvec3 up{ 0, 0, 1 };
    m_legSkel = {};
    m_hipDown = 0.0;
    for (int i = 0; i < 2; i++)
    {
        const int32_t bones[4] = { m_upLegs[i], m_legs[i], m_feet[i], m_toes[i] };
        const glm::dvec3 y[4] = { refAt(m_legs[i]) - refAt(m_upLegs[i]), refAt(m_feet[i]) - refAt(m_legs[i]), forward, forward };
        const glm::dvec3 z[4] = { forward, forward, up, up };
        for (int b = 0; b < 4; b++)
        {
            const glm::dquat inverse = glm::inverse(m_refPose[bones[b]].rotation);
            m_legAxes[i].y[b] = glm::normalize(inverse * y[b]);
            m_legAxes[i].z[b] = glm::normalize(inverse * z[b]);
        }
        m_hipJointsLocal[i] = Relative(m_refPose[m_upLegs[i]], m_refPose[m_hips]);
        m_legSkel.thigh += 0.5 * glm::length(y[0]) * CmToM;
        m_legSkel.shin += 0.5 * glm::length(y[1]) * CmToM;
        m_legSkel.ankleHeight += 0.5 * refAt(m_feet[i]).z * CmToM;
        m_legSkel.toeForward += 0.5 * glm::dot(refAt(m_toes[i]) - refAt(m_feet[i]), forward) * CmToM;
        m_legSkel.toeHeight += 0.5 * refAt(m_toes[i]).z * CmToM;
        m_hipDown += 0.5 * (refAt(m_hips).z - refAt(m_upLegs[i]).z) * CmToM;
    }
    uevr::API::get()->log_info("[MGSDelta] BodyIK: legs: thigh %.1f cm, shin %.1f, ankle %.1f high, ball of the foot %.1f ahead and %.1f high, hip joints %.1f below the hips",
        m_legSkel.thigh / CmToM, m_legSkel.shin / CmToM, m_legSkel.ankleHeight / CmToM, m_legSkel.toeForward / CmToM, m_legSkel.toeHeight / CmToM, m_hipDown / CmToM);

    m_isVirtual.assign(m_numBones, false);
    for (int32_t i = 0; i < m_numBones; i++)
        m_isVirtual[i] = names[i].ToString().rfind("VB ", 0) == 0;

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
    // Outside the player's control (cutscenes, menus), the game's animation
    if (pose == nullptr || mesh == nullptr || !m_playerControlled)
        return;

    const auto now = std::chrono::steady_clock::now();
    const double dt = m_lastSolve == std::chrono::steady_clock::time_point{} ? 0.0
        : std::clamp(std::chrono::duration<double>(now - m_lastSolve).count(), 0.0, MaxDeltaTime);
    m_lastSolve = now;

    const Transform meshToWorld = ToTransform(ue::call<ue::FTransform>(mesh, L"K2_GetComponentToWorld"));
    // The game's head, which the camera follows (ToAnimatedHead)
    const Transform animatedHead = ToTransform(pose[m_head]);

    // The game's animation is set aside: every bone starts from the reference pose, and the IK places
    // the body from there (the virtual bones aside: the game derives them from the others)
    for (int32_t bone = 0; bone < m_numBones; bone++)
    {
        if (m_isVirtual[bone])
            continue;
        pose[bone].Rotation = ToUe(m_refPose[bone].rotation);
        pose[bone].Translation = ToUe(m_refPose[bone].translation);
        pose[bone].Scale3D = ToUe(m_refPose[bone].scale);
    }

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

    SolveBody(pose, meshToWorld, hands, scale, dt);

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

void BodyIK::SolveBody(ue::FTransform* pose, const Transform& meshToWorld, const std::optional<Transform> hands[2], double scale, double dt)
{
    auto* hmdActor = m_hmd.get();
    auto* hmdRoot = hmdActor != nullptr ? ue::get<ue::UObject*>(hmdActor, L"RootComponent") : nullptr;
    if (hmdRoot == nullptr)
    {
        m_torso.reset();
        m_root.reset();
        m_lastEyes.reset();
        m_footing.reset();
        m_lastCom.reset();
        m_hipDrop = 0.0;
        return;
    }

    // The headset in mesh space (cm): UEVR moves this actor with it. Its x looks forward, z up.
    const Transform hmdWorld = FollowCamera(ToTransform(ue::call<ue::FTransform>(hmdRoot, L"K2_GetComponentToWorld")));
    const Transform hmd = Relative(hmdWorld, meshToWorld);
    m_hmdSolved = hmdWorld.translation;
    const glm::dvec3 hmdForward = hmd.rotation * glm::dvec3{ 1, 0, 0 };
    const glm::dvec3 hmdUp = hmd.rotation * glm::dvec3{ 0, 0, 1 };
    const glm::dvec3 eyes = hmd.translation + hmd.rotation * glm::dvec3{ HeadOffset[0], HeadOffset[1], HeadOffset[2] };

    const int32_t bones[6] = { m_hips, m_spine, m_spine1, m_spine2, m_neck, m_head };

    // The head turns with the headset, and goes where Snake's eyes are at the camera
    const glm::dquat headRot = AlignPair(*m_headForwardLocal, m_headUpLocal, hmdForward, hmdUp);
    const glm::dvec3 headToEyes = m_eyesLocal ? headRot * *m_eyesLocal
        : (hmdUp * HeadPivotDown + hmdForward * HeadPivotBack) * scale / CmToM;
    const glm::dvec3 headJoint = eyes - headToEyes;

    // Between mesh space and the world, the IK's axes (m). The yaws below are filtered in the world:
    // the mesh turns with the pawn, which the game and the player controller turn back and forth.
    auto toWorld = [&](const glm::dvec3& v) { return ToIK(Compose(Transform{ {}, FromIK(v) / CmToM, { 1, 1, 1 } }, meshToWorld).translation * CmToM); };
    auto toMesh = [&](const glm::dvec3& v) { return ToIK(Relative(Transform{ {}, FromIK(v) / CmToM, { 1, 1, 1 } }, meshToWorld).translation * CmToM); };
    const glm::dvec3 meshForward = meshToWorld.rotation * glm::dvec3{ 1, 0, 0 };
    const double meshYaw = std::atan2(meshForward.y, meshForward.x);

    // ---- torso yaw: where the hands and head have the chest face (chasm's torso.rs) ----
    const glm::dvec3 forwardIK = ToIK(hmdForward), upIK = ToIK(hmdUp);
    // Looking straight down, the top of the head still points the way you face
    glm::dvec3 facing = forwardIK - upIK * forwardIK.y;
    facing.y = 0.0;
    const double headYaw = glm::length(facing) > 1e-4 ? ik::YawOf(facing) + meshYaw : (m_torso ? m_torso->Yaw() : meshYaw);
    if (!m_torso)
        m_torso.emplace(headYaw);
    std::optional<glm::dvec3> handsIK[2];
    for (int i = 0; i < 2; i++)
    {
        if (hands[i])
            handsIK[i] = toWorld(ToIK(hands[i]->translation * CmToM));
    }
    const double torsoYawWorld = m_torso->Update(m_torsoParams, dt, scale, toWorld(ToIK(eyes * CmToM)), toWorld(ToIK(headJoint * CmToM)), headYaw, handsIK);
    const double torsoYaw = ik::WrapAngle(torsoYawWorld - meshYaw);

    // ---- root yaw: the hips trail the torso ----
    if (m_lastEyes && dt > 0.0)
    {
        const glm::dvec3 moved = hmdWorld.translation - *m_lastEyes;
        const double speed = std::hypot(moved.x, moved.y) * CmToM / dt;
        m_eyesSpeed += (speed - m_eyesSpeed) * ik::Smoothing(dt, VelocityTau);
    }
    m_lastEyes = hmdWorld.translation;
    if (!m_root)
        m_root.emplace(torsoYawWorld);
    const bool moving = m_eyesSpeed > MovingSpeed || (m_footing && m_footing->Walking());
    const double rootYaw = ik::WrapAngle(m_root->Update(dt, torsoYawWorld, moving) - meshYaw);

    ik::SpineSkeleton skel = m_spineSkel;
    skel.scale = scale;
    ik::LegSkeleton legs = m_legSkel;
    legs.scale = scale;

    // ---- pitch: how far and how the head has dropped. The neck's drop, not the eyes': nodding swings
    // the eyes about the neck without the body going anywhere. ----
    const double eyesUp = m_eyesLocal ? glm::dot(*m_eyesLocal, m_headUpLocal) * CmToM : HeadPivotDown * scale;
    const double levelEyes = headJoint.z * CmToM + eyesUp; // the floor is at the mesh's origin
    // Standing straight: the legs all but straight, the chain upright from the hips to the eyes
    const double standingEyes = legs.ankleHeight + (legs.thigh + legs.shin) * LegSlack + m_hipDown + skel.spine[0] + skel.spine[1]
        + skel.spine[2] + skel.spine[3] + skel.neck * std::cos(ik::NeckLean) + eyesUp;
    const double headPitch = std::asin(std::clamp(-forwardIK.y, -1.0, 1.0));
    const double targetPitch = m_crawling ? PronePitch : ik::TargetPitch(m_spineParams, skel, levelEyes, standingEyes, headPitch);
    m_pitch.Step(targetPitch - m_pitch.value, dt, m_crawling ? ProneTau : m_spineParams.tau);

    ik::ChainInput input;
    input.headJoint = ToIK(headJoint * CmToM);
    input.headRot = ik::FrameYZ(upIK, forwardIK, forwardIK);
    input.torsoYaw = torsoYaw;
    input.rootYaw = rootYaw;
    input.lying = m_crawling;
    auto chainAt = [&](double pitch) {
        ik::ChainInput at = input;
        at.pitch = pitch;
        return ik::BuildChain(skel, at);
    };
    ik::Chain chain = chainAt(m_pitch.value);

    // A chain bone onto its solved segment: its way up along it, the body's right as near as it goes
    auto chainBone = [&](const ik::Chain& c, int k) {
        const auto& axes = (*m_segmentAxes)[k];
        const glm::dvec3 up = FromIK(c.rot[k] * glm::dvec3{ 0, 1, 0 });
        const glm::dvec3 right = FromIK(c.rot[k] * glm::dvec3{ 1, 0, 0 });
        return Transform{ AlignPair(axes.up, axes.right, up, right), FromIK(c.pos[k]) / CmToM, ToGlm(pose[bones[k]].Scale3D) };
    };
    // The hip joints on a chain (the IK's axes, m)
    auto hipJoints = [&](const ik::Chain& c) {
        const Transform hips = chainBone(c, 0);
        std::array<glm::dvec3, 2> joints;
        for (int i = 0; i < 2; i++)
            joints[i] = ToIK(Compose(m_hipJointsLocal[i], hips).translation * CmToM);
        return joints;
    };
    auto flat =[](const glm::dvec3& v) { return glm::dvec3{ v.x, 0.0, v.z }; };

    // Crawling, the body lies flat behind the head, the legs straight back from the hips (the
    // reference pose): no feet
    ik::Foot feet[2];
    double lifts[2] = { 0.0, 0.0 };
    if (m_crawling)
    {
        m_footing.reset();
        m_lastCom.reset();
        m_hipDrop = 0.0;
    }
    else
    {
        // ---- feet: planted in the world ----
        const glm::dvec3 com = toWorld(glm::mix(chain.pos[0], input.headJoint, ComUpTheSpine));
        if (m_lastCom && dt > 0.0)
            m_comVel = glm::mix(m_comVel, (com - *m_lastCom) / dt, ik::Smoothing(dt, VelocityTau));
        m_lastCom = com;
        const double rootYawWorld = rootYaw + meshYaw;
        if (!m_footing || glm::length(flat(com - 0.5 * (m_footing->feet[0].pos + m_footing->feet[1].pos))) > FeetResetDistance)
        {
            m_footing.emplace(m_legParams, legs, com, rootYawWorld);
            m_comVel = {};
        }
        ik::FeetInput feetInput;
        feetInput.com = com;
        feetInput.comVel = m_comVel;
        feetInput.rootYaw = rootYawWorld;
        const auto joints = hipJoints(chain);
        for (int i = 0; i < 2; i++)
            feetInput.hips[i] = toWorld(joints[i]);
        feetInput.hipsYaw = chain.yaw[0] + meshYaw;
        m_footing->Update(m_legParams, legs, dt, feetInput);
        // In mesh space, on the floor under the pawn
        for (int i = 0; i < 2; i++)
        {
            feet[i] = m_footing->feet[i];
            feet[i].pos = flat(toMesh(feet[i].pos));
            feet[i].yaw = ik::WrapAngle(feet[i].yaw - meshYaw);
            lifts[i] = feet[i].Lift(m_legParams, scale);
        }

        // ---- hips: keep both feet planted ----
        auto hipsRange = [&](const ik::Chain& c) {
            const auto j = hipJoints(c);
            double low = -std::numeric_limits<double>::max(), high = std::numeric_limits<double>::max();
            for (int i = 0; i < 2; i++)
            {
                const auto [lo, hi] = ik::HipHeightRange(m_legParams, legs, j[i], feet[i], lifts[i]);
                const double offset = j[i].y - c.pos[0].y;
                low = std::max(low, lo - offset);
                high = std::min(high, hi - offset);
            }
            return std::pair{ low, high };
        };
        const double tooLow = hipsRange(chain).first;
        if (chain.pos[0].y < tooLow)
        {
            // Too low for the knees: bend the spine further (that raises the hips), then squash it
            double lo = m_pitch.value, hi = ik::MaxPitch;
            if (chainAt(hi).pos[0].y >= tooLow)
            {
                for (int i = 0; i < 12; i++)
                {
                    const double mid = 0.5 * (lo + hi);
                    if (chainAt(mid).pos[0].y >= tooLow)
                        hi = mid;
                    else
                        lo = mid;
                }
            }
            // (Where the knees can't bend further, the spine bends at once)
            m_pitch = ik::Follow{ hi, 0.0 };
            chain = chainAt(hi);
            const double shortBy = tooLow - chain.pos[0].y;
            if (shortBy > 0.0)
                ik::ShiftHips(chain, -shortBy);
        }
        const auto [low, high] = hipsRange(chain);
        // Too high to reach the feet: pull the hips down (the spine stretches). The pull is held and let go
        // slowly so a walking gait doesn't bob the body every step.
        const double excess = std::max(chain.pos[0].y - high, 0.0);
        m_hipDrop = excess >= m_hipDrop ? excess : m_hipDrop + (excess - m_hipDrop) * ik::Smoothing(dt, m_legParams.hipReleaseTau);
        const double drop = std::min(m_hipDrop, std::max(chain.pos[0].y - low, 0.0));
        if (drop > 0.0)
            ik::ShiftHips(chain, drop);
    }

    // ---- write it all: the chain, the head, the legs, the shoulders ----
    BoneOverride overrides[16];
    size_t count = 0;
    for (int k = 0; k < 5; k++)
    {
        const Transform bone = chainBone(chain, k);
        overrides[count++] = { bones[k], bone.rotation, bone.translation };
    }
    overrides[count++] = { m_head, headRot, FromIK(chain.pos[5]) / CmToM };
    const glm::dvec3 hipsForward = chain.rot[0] * glm::dvec3{ 0, 0, 1 };
    const auto legJoints = hipJoints(chain);
    for (int i = 0; i < 2 && !m_crawling; i++)
    {
        const ik::LegPose leg = ik::SolveLeg(m_legParams, legs, legJoints[i], feet[i], lifts[i], hipsForward);
        const int32_t legBones[4] = { m_upLegs[i], m_legs[i], m_feet[i], m_toes[i] };
        const glm::dvec3 positions[4] = { leg.hip, leg.knee, leg.ankle, leg.toes };
        for (int b = 0; b < 4; b++)
        {
            const glm::dquat rotation = AlignPair(m_legAxes[i].y[b], m_legAxes[i].z[b], FromIK(leg.rot[b] * glm::dvec3{ 0, 1, 0 }),
                FromIK(leg.rot[b] * glm::dvec3{ 0, 0, 1 }));
            overrides[count++] = { legBones[b], rotation, FromIK(positions[b]) / CmToM };
        }
    }
    // The shoulders as in the reference pose, square on the chest: the game's are hunched forward
    const Transform spine2{ overrides[3].rotation, overrides[3].translation, ToGlm(pose[m_spine2].Scale3D) };
    for (int i = 0; i < 2; i++)
    {
        const int32_t clavicle = m_arms[i].clavicle;
        const Transform shoulder = Compose(Relative(m_refPose[clavicle], m_refPose[m_spine2]), spine2);
        overrides[count++] = { clavicle, shoulder.rotation, shoulder.translation };
    }
    WriteSubtree(pose, m_hipsSubtree, overrides, count);
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

    // The shoulder girdle: the clavicle turns to carry the shoulder joint where it goes
    {
        const glm::dvec3 root = at(arm.clavicle), rest = at(arm.upper);
        ik::ArmSkeleton skel;
        skel.upperArm = glm::distance(rest, at(arm.lower));
        skel.lowerArm = glm::distance(at(arm.lower), at(arm.hand));
        skel.scale = skel.ArmLength() / ReferenceArmLength;
        const glm::dvec3 placed = ik::PlaceShoulder(m_armParams, skel, dt, arm.state, arm.side, chest, root, rest, targetPos * CmToM);
        const glm::dquat turn = glm::rotation(glm::normalize(rest - root), glm::normalize(placed - root));
        const BoneOverride clavicle = { arm.clavicle, glm::normalize(turn * ToGlm(pose[arm.clavicle].Rotation)), ToGlm(pose[arm.clavicle].Translation) };
        WriteSubtree(pose, arm.clavicleSubtree, &clavicle, 1);
    }

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

void BodyIK::DrawMenu(bool* enabled, bool& settingsModified)
{
    auto* self = s_instance;
    if (self == nullptr || !ImGui::CollapsingHeader("Full body IK (experimental)"))
        return;

    if (ImGui::Checkbox("Enable", enabled))
        settingsModified = true;
    ImGui::SliderFloat3("Head offset (forward, right, up, cm)", self->HeadOffset, -30.0f, 30.0f, "%.1f");
    ImGui::Text("Mesh bound: %s, bones: %d, pose buffers: %s", self->m_mesh.get() ? "yes" : "no", self->m_numBones,
        self->m_readIndexOffset >= 0 ? "found" : "not found");
    ImGui::Text("VR hands: left %s, right %s", self->m_arms[0].targetHasHand ? "found" : "not found",
        self->m_arms[1].targetHasHand ? "found" : "not found");

    // Snake's eyes as solved against the camera UEVR renders from, in the camera's frame
    if (self->m_eyesLocal && std::chrono::duration<double>(std::chrono::steady_clock::now() - self->m_headSolvedTime).count() < HeadMapLifetime)
    {
        const glm::dvec3 eyes = Compose(Transform{ {}, *self->m_eyesLocal, { 1, 1, 1 } }, self->m_headSolved).translation;
        const glm::dvec3 camera = 0.5 * (self->m_views[0] + self->m_views[1]);
        const glm::dquat cameraToWorld = RotatorToQuat(self->m_viewRotation.x, self->m_viewRotation.y, self->m_viewRotation.z);
        const glm::dvec3 off = glm::inverse(cameraToWorld) * (eyes - camera);
        const glm::dvec3 hmdOff = glm::inverse(cameraToWorld) * (self->m_hmdSolved - camera);
        ImGui::Text("Snake's eyes from the camera: %.1f cm forward, %.1f right, %.1f up", off.x, off.y, off.z);
        ImGui::Text("Headset followed, from the camera: %.1f cm forward, %.1f right, %.1f up", hmdOff.x, hmdOff.y, hmdOff.z);
    }
}

void BodyIK::OnViewComputed(int viewIndex, const glm::dvec3& position, const glm::dvec3& rotation)
{
    if (viewIndex == 0 || viewIndex == 1)
        m_views[viewIndex] = position;
    m_viewRotation = rotation;
}
