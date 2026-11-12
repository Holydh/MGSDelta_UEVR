#include "BodyIK.hpp"

#include <map>
#include <utility>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "renderlib/imgui/imgui.h"

#include "HookManager.hpp"
#include "Logger.hpp"

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

} // namespace

void BodyIK::Update(HookManager& hookManager)
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
}

void BodyIK::Cleanup()
{
    m_mesh.reset();
    m_failedMesh.reset();
    m_animInstance.reset();
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
    m_rightArm = boneIndex(L"BaseBody_RightArm");
    m_rightForeArm = boneIndex(L"BaseBody_RightForeArm");
    m_rightHand = boneIndex(L"BaseBody_RightHand");
    if (m_rightArm < 0 || m_rightForeArm < 0 || m_rightHand < 0)
        return false;

    std::vector<bool> inSubtree(m_numBones, false);
    m_rightForeArmSubtree.clear();
    for (int32_t i = m_rightForeArm; i < m_numBones; i++)
    {
        inSubtree[i] = i == m_rightForeArm || inSubtree[m_parents[i]];
        if (inSubtree[i])
            m_rightForeArmSubtree.push_back(i);
    }

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
    const auto expected = ToGlm(ue::call<ue::FTransform>(mesh, L"GetBoneTransform",
        ue::call<ue::FName>(mesh, L"GetBoneName", m_rightHand), ue::ERelativeTransformSpace::RTS_Component).Translation);
    double distances[2];
    for (int i = 0; i < 2; i++)
        distances[i] = glm::length(ToGlm(static_cast<const ue::FTransform*>(arrays[i].data)[m_rightHand].Translation) - expected);
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
    if (auto* pose = read >= 0 ? PoseBuffer(1 - read) : nullptr)
        BendForearm(pose);
}

void BodyIK::BendForearm(ue::FTransform* pose)
{
    // Step 1 test: bend the right forearm at the elbow
    const glm::dvec3 shoulder = ToGlm(pose[m_rightArm].Translation);
    const glm::dvec3 elbow = ToGlm(pose[m_rightForeArm].Translation);
    const glm::dvec3 wrist = ToGlm(pose[m_rightHand].Translation);
    glm::dvec3 axis = glm::cross(elbow - shoulder, wrist - elbow);
    axis = glm::length(axis) > 1e-6 ? glm::normalize(axis) : glm::dvec3{ 0.0, 0.0, 1.0 };
    const glm::dquat bend = glm::angleAxis(glm::radians(static_cast<double>(TestElbowAngle)), axis);

    // The whole subtree turns rigidly around the elbow
    for (const int32_t bone : m_rightForeArmSubtree)
    {
        auto& t = pose[bone];
        t.Translation = ToUe(elbow + bend * (ToGlm(t.Translation) - elbow));
        t.Rotation = ToUe(glm::normalize(bend * ToGlm(t.Rotation)));
    }
}

void BodyIK::DrawMenu()
{
    auto* self = s_instance;
    if (self == nullptr || !ImGui::CollapsingHeader("Full body IK (experimental)"))
        return;

    ImGui::Checkbox("Enable", &self->Enabled);
    ImGui::SliderFloat("Test: bend right forearm (degrees)", &self->TestElbowAngle, -90.0f, 90.0f);
    ImGui::Text("Mesh bound: %s, bones: %d, pose buffers: %s", self->m_mesh.get() ? "yes" : "no", self->m_numBones,
        self->m_readIndexOffset >= 0 ? "found" : "not found");
}
