#pragma once
// Full body IK (work in progress).
//
// Snake's visible meshes (body, arms, hands, gear, weapons) all follow one leader mesh, the
// character's own skeletal mesh component. Once the game has evaluated its animation for the
// frame, its pose is overwritten here, in that component's component-space bone transforms,
// so every follower mesh follows.
#include <cstdint>
#include <vector>

#include "uevr/API.hpp"
#include "ue/Reflection.hpp"

class HookManager;

class BodyIK {
public:
    BodyIK() { s_instance = this; }

    // Game thread, every frame: binds to the player's mesh and installs the hook
    void Update(HookManager& hookManager);
    // Level change: everything bound to the previous world is gone
    void Cleanup();

    static void DrawMenu();

    bool Enabled = false;
    float TestElbowAngle = 45.0f; // step 1 test: degrees the right forearm is bent by

private:
    static inline BodyIK* s_instance = nullptr;
    static bool OnPostEvaluateAnimation(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);

    enum class PoseBuffers { Found, Retry, NotFound };

    bool Bind(ue::UObject* mesh);
    PoseBuffers FindPoseBuffers(ue::UObject* mesh);
    int32_t ReadIndex() const;                     // -1 while unknown
    ue::FTransform* PoseBuffer(int32_t index) const; // nullptr for an invalid index
    void ApplyPose();
    void BendForearm(ue::FTransform* pose);

    ue::WeakObject m_mesh;
    ue::WeakObject m_failedMesh; // not bound again until it changes: binding reads the whole skeleton
    ue::WeakObject m_animInstance;
    bool m_hookInstalled = false;

    int32_t m_numBones = 0;
    std::vector<int32_t> m_parents;      // parent bone index, -1 for the root
    // Offsets in the mesh component (not reflected, engine layout): its two component-space pose
    // buffers (TArray<FTransform>[2]) and the index of the one holding the current pose.
    int32_t m_poseArraysOffset = -1;
    int32_t m_readIndexOffset = -1;
    int32_t m_poseBufferAttempts = 0;
    struct { double distances[2]; int32_t indices[2]; } m_lastMismatch{}; // for the error message

    int32_t m_rightArm = -1, m_rightForeArm = -1, m_rightHand = -1;
    std::vector<int32_t> m_rightForeArmSubtree; // the forearm and every bone below it
};
