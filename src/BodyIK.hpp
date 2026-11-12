#pragma once
// Full body IK (work in progress).
//
// Snake's visible meshes (body, arms, hands, gear, weapons) all follow one leader mesh, the
// character's own skeletal mesh component. Once the game has evaluated its animation for the
// frame, its pose is overwritten here, in that component's component-space bone transforms,
// so every follower mesh follows.
//
// Step 2: the arms reach for the VR hands (the Lua hands on the motion controllers, which are
// copies of Snake's arm mesh placed with the profile's hand offsets).
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include "uevr/API.hpp"
#include "ue/Reflection.hpp"
#include "ik/ArmIK.hpp"

class HookManager;
class PlayerManager;

class BodyIK {
public:
    BodyIK() { s_instance = this; }

    // Game thread, every frame: binds to the player's mesh, picks up the VR hands, installs the hook
    void Update(HookManager& hookManager, const PlayerManager& playerManager);
    // Level change: everything bound to the previous world is gone
    void Cleanup();

    static void DrawMenu();

    bool Enabled = false;

    // Rotation, translation and scale, composed like the engine's FTransform
    struct Transform {
        glm::dquat rotation{ 1, 0, 0, 0 };
        glm::dvec3 translation{};
        glm::dvec3 scale{ 1, 1, 1 };
    };

private:
    struct Arm {
        ik::Side side = ik::Side::Right;
        int32_t upper = -1, lower = -1, hand = -1;   // BaseBody_*Arm, *ForeArm, *Hand
        int32_t index = -1, middle = -1, pinky = -1; // first finger joints: give the hand's axes
        std::vector<int32_t> subtree;                // the upper arm and every bone below it
        ue::FName handName;
        ue::WeakObject target;                       // the VR hand mesh on this side's controller
        bool targetHasHand = false;
        ue::WeakObject controller;                   // this side's motion controller
        // The VR hand mesh relative to the controller, as last seen attached to it. The mod moves the
        // hand onto the weapon, which hangs from Snake's hand: read from there, the target would
        // follow Snake's hand and the arm would chase itself.
        std::optional<Transform> handOnController;
        ik::ArmState state;
        std::optional<glm::dvec3> hingeLocal;        // elbow hinge axis, in the upper arm's frame
    };

    static inline BodyIK* s_instance = nullptr;
    static bool OnPostEvaluateAnimation(uevr::API::UFunction* fn, uevr::API::UObject* obj, void* locals, void* result);

    enum class PoseBuffers { Found, Retry, NotFound };

    bool Bind(ue::UObject* mesh);
    PoseBuffers FindPoseBuffers(ue::UObject* mesh);
    int32_t ReadIndex() const;                       // -1 while unknown
    ue::FTransform* PoseBuffer(int32_t index) const; // nullptr for an invalid index
    void UpdateTarget(Arm& arm, ue::UObject* handMesh, ue::UObject* controller);
    // Where the VR hand's hand bone is in the world, placed from the controller
    std::optional<Transform> TargetHand(Arm& arm) const;
    void ApplyPose();
    void SolveArm(Arm& arm, ue::FTransform* pose, const ik::ChestFrame& chest, const ue::FTransform& meshToWorld, double dt);
    // Sets the given bones of a subtree (parents before children) and carries every other bone of it along
    struct BoneOverride { int32_t bone; glm::dquat rotation; glm::dvec3 translation; };
    void WriteSubtree(ue::FTransform* pose, const std::vector<int32_t>& subtree, const BoneOverride* overrides, size_t count);

    ue::WeakObject m_mesh;
    ue::WeakObject m_failedMesh; // not bound again until it changes: binding reads the whole skeleton
    ue::WeakObject m_animInstance;
    bool m_hookInstalled = false;

    int32_t m_numBones = 0;
    std::vector<int32_t> m_parents;      // parent bone index, -1 for the root
    std::vector<ue::FTransform> m_scratch; // the pose before it's written
    // Offsets in the mesh component (not reflected, engine layout): its two component-space pose
    // buffers (TArray<FTransform>[2]) and the index of the one holding the current pose.
    int32_t m_poseArraysOffset = -1;
    int32_t m_readIndexOffset = -1;
    int32_t m_poseBufferAttempts = 0;
    struct { double distances[2]; int32_t indices[2]; } m_lastMismatch{}; // for the error message

    Arm m_arms[2]; // [left, right]
    int32_t m_spine2 = -1, m_neck = -1;
    ik::ArmParams m_armParams;
    std::chrono::steady_clock::time_point m_lastSolve{};
};
