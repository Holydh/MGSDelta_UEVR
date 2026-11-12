#pragma once
// Full body IK (work in progress).
//
// Snake's visible meshes (body, arms, hands, gear, weapons) all follow one leader mesh, the
// character's own skeletal mesh component. Once the game has evaluated its animation for the
// frame, its pose is overwritten here, in that component's component-space bone transforms,
// so every follower mesh follows.
//
// The head goes under the headset (Snake's eyes where the camera is), the spine hangs from it down to
// the hips, and the arms reach for the VR hands (the Lua hands on the motion controllers, which are
// copies of Snake's arm mesh placed with the profile's hand offsets). The feet are planted in the
// world and step or walk as the body goes, the legs reach them, and the hips come down as far as the
// feet need. Crawling, the trunk lies flat behind the head, the legs straight back. The game's own
// animation is set aside meanwhile: the body starts from the reference pose.
#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include "uevr/API.hpp"
#include "ue/Reflection.hpp"
#include "ik/ArmIK.hpp"
#include "ik/LegsIK.hpp"
#include "ik/SpineIK.hpp"
#include "ik/TorsoIK.hpp"

class HookManager;
class PlayerManager;

class BodyIK {
public:
    BodyIK() { s_instance = this; }

    // Game thread, every frame, after the camera is computed: binds to the player's mesh, picks up the
    // VR hands and the headset (an actor UEVR moves with it), installs the hook. cameraPosition and
    // cameraRotation: the VR camera's origin this frame, as given to UEVR.
    void Update(HookManager& hookManager, const PlayerManager& playerManager, ue::UObject* hmdActor,
                const UEVR_Vector3d& cameraPosition, const UEVR_Rotatord& cameraRotation);
    // Level change: everything bound to the previous world is gone
    void Cleanup();

    // enabled: the saved setting, settingsModified: set when it changes
    static void DrawMenu(bool* enabled, bool& settingsModified);
    // Where UEVR renders an eye from, once the headset is applied: shown against Snake's eyes in the menu
    void OnViewComputed(int viewIndex, const glm::dvec3& position, const glm::dvec3& rotation);

    // A point attached to Snake's head (e.g. a component on its head socket), moved to where it would
    // be on the head as the game animated it. The camera is placed from that head: read from the
    // head under the headset, it would follow the headset and push itself away.
    static ue::FVector ToAnimatedHead(const ue::FVector& worldPoint);

    bool Enabled = true; // follows the setting (MGSDeltaPlugin)
    // Snake's eyes from the camera, in the headset's frame: forward, right, up (cm)
    float HeadOffset[3] = { 16.3f, 0.0f, -4.8f };

    // Rotation, translation and scale, composed like the engine's FTransform
    struct Transform {
        glm::dquat rotation{ 1, 0, 0, 0 };
        glm::dvec3 translation{};
        glm::dvec3 scale{ 1, 1, 1 };
    };

private:
    struct Arm {
        ik::Side side = ik::Side::Right;
        int32_t clavicle = -1;                       // BaseBody_*Shoulder
        int32_t upper = -1, lower = -1, hand = -1;   // BaseBody_*Arm, *ForeArm, *Hand
        int32_t index = -1, middle = -1, pinky = -1; // first finger joints: give the hand's axes
        std::vector<int32_t> clavicleSubtree;        // the clavicle and every bone below it
        std::vector<int32_t> subtree;                // the upper arm and every bone below it
        int32_t roll[2] = { -1, -1 };                // forearm twist bones (*ForeArmRoll1, *ForeArmRoll2)
        std::vector<int32_t> rollSubtree[2];
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
    // In first person the game hides Snake's model every frame, through its model parts controller: his
    // body's parts are taken from it meanwhile, and shown as they were in the normal view.
    // bodyShown: Snake's body is shown in VR.
    void KeepBodyVisible(ue::UObject* pawn, bool bodyShown);
    // Gives the body's parts back to the model parts controller
    void ReleaseBodyParts();
    void FindEyes(ue::UObject* pawn);
    // UEVR places the controllers and the headset from the camera's origin when the view is computed,
    // after Snake's animation: during it they still hang from last frame's origin. This moves a
    // transform from last frame's origin to this frame's.
    Transform FollowCamera(const Transform& world) const;
    // Where the VR hand's hand bone is in the world, placed from the controller
    std::optional<Transform> TargetHand(Arm& arm) const;
    void ApplyPose();
    // The hips, spine, head and legs, in mesh space. hands: the hand targets [left, right], when tracked.
    void SolveBody(ue::FTransform* pose, const Transform& meshToWorld, const std::optional<Transform> hands[2], double scale, double dt);
    void SolveArm(Arm& arm, ue::FTransform* pose, const ik::ChestFrame& chest, const Transform& target, double dt);
    // Spreads the hand's roll about the forearm over the forearm's twist bones
    void TwistForearm(Arm& arm, ue::FTransform* pose, const glm::dquat& animLowerRot, const glm::dquat& animHandRot);
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
    // BaseBody_Hips, Spine, Spine1, Spine2, Neck, Head: the chain hung from the headset
    int32_t m_hips = -1, m_spine = -1, m_spine1 = -1, m_spine2 = -1, m_neck = -1, m_head = -1;
    int32_t m_headEnd = -1;
    // BaseBody_*UpLeg, *Leg, *Foot, *FootFinger1 (the ball of the foot) [left, right]
    int32_t m_upLegs[2] = { -1, -1 }, m_legs[2] = { -1, -1 }, m_feet[2] = { -1, -1 }, m_toes[2] = { -1, -1 };
    std::vector<int32_t> m_hipsSubtree; // BaseBody_Hips and every bone below it
    ue::WeakObject m_hmd;
    // While the headset is followed: the torso's yaw (mesh space), the hips' (world), the spine's pitch
    std::optional<ik::TorsoYaw> m_torso;
    std::optional<ik::RootYaw> m_root;
    ik::Follow m_pitch;
    // How fast the eyes go over the ground (m/s, smoothed), from where they were last frame (world)
    double m_eyesSpeed = 0.0;
    std::optional<glm::dvec3> m_lastEyes;
    // From the reference pose, in each chain bone's frame (Hips to Neck): the way up to the next joint
    // and the body's right
    struct SegmentAxes { glm::dvec3 up, right; };
    std::optional<std::array<SegmentAxes, 5>> m_segmentAxes;
    // From the reference pose, in each leg bone's frame (UpLeg, Leg, Foot, toes): the solved frame's
    // +y (along the leg, or towards the toes) and +z (forward, or up)
    struct LegAxes { glm::dvec3 y[4], z[4]; };
    LegAxes m_legAxes[2];
    Transform m_hipJointsLocal[2];  // the UpLegs in the hips bone's frame, as in the reference pose
    ik::SpineSkeleton m_spineSkel{}; // Snake's lengths, from the reference pose (scale set per frame)
    ik::LegSkeleton m_legSkel;
    double m_hipDown = 0.0;          // the hip joints below the hips joint (m)
    // The feet, planted in the world (the IK's axes, m), and the centre of mass they balance
    std::optional<ik::Feet> m_footing;
    std::optional<glm::dvec3> m_lastCom;
    glm::dvec3 m_comVel{};
    double m_hipDrop = 0.0;          // how far the hips are pulled down to keep the feet planted (m)
    bool m_crawling = false;
    bool m_playerControlled = false;
    std::vector<bool> m_isVirtual; // the skeleton's virtual bones ("VB ..."), which the game derives from others
    std::optional<glm::dvec3> m_headForwardLocal; // the way the face looks, in the head bone's frame
    glm::dvec3 m_headUpLocal{};                   // up to the top of the head, in the head bone's frame
    std::vector<Transform> m_refPose;             // the skeleton's reference pose (component space)
    ue::FName m_headName;
    // Between Snake's eyes, in the head bone's frame (cm), from his facial mesh. Without it, chasm's
    // average head pivot is used.
    std::optional<glm::dvec3> m_eyesLocal;
    bool m_eyesSearched = false;
    ik::ArmParams m_armParams;
    ik::TorsoParams m_torsoParams;
    ik::SpineParams m_spineParams;
    ik::LegParams m_legParams;
    std::chrono::steady_clock::time_point m_lastSolve{};

    // The VR camera's origin, this frame and last frame
    Transform m_cameraOrigin, m_previousCameraOrigin;
    bool m_hasCameraOrigin = false;
    // The parts registered in Snake's model parts controller, head parts aside, as last seen in the normal view
    struct BodyPart { ue::WeakObject mesh; uint32_t partBit; bool visible; };
    std::vector<BodyPart> m_bodyParts;
    ue::WeakObject m_partsController;
    bool m_bodyPartsTaken = false;

    // Snake's head as animated and as solved, last frame (world): for ToAnimatedHead
    Transform m_headAnimated, m_headSolved;
    std::chrono::steady_clock::time_point m_headSolvedTime{};
    glm::dvec3 m_hmdSolved{};     // the headset as the last solve had it (world)
    glm::dvec3 m_views[2]{};      // the rendered eyes' positions (world)
    glm::dvec3 m_viewRotation{};  // pitch, yaw, roll (degrees)
};
