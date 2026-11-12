#pragma once
// Feet and legs. Feet are anchored to the world: while standing they step when balance needs it
// (VRIK's procedural footsteps); going somewhere, by the stick or walking about the room, a
// velocity-driven gait takes over (FRIK's phase generator), and faster, a run with the feet in the
// air most of the time (not in chasm's: its walk's stride grows a whole leg long at running speed).
// Legs are two-bone IK to the planted
// feet; a foot out of reach rises onto its toes before the hips come down (VRIK plants the toes,
// not the ankles).
//
// Ported from chasm's VR framework (crates/ik/src/legs.rs,
// https://github.com/chasmlol/chasms-vr-framework). MIT License, Copyright (c) 2026 chasmlol: see
// ArmIK.hpp for the full notice.
//
// Units are metres. Positions are in the original's convention: x right, y up, z forward, and a yaw
// of 0 faces +z, positive towards +x. The floor is at y = 0.
#include <optional>
#include <utility>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace ik {

struct LegParams {
    double footSpacing = 0.3;      // between the feet standing (m at reference size)
    // A foot steps when: this far from its place whatever the rest; its ankle further from under its
    // hip joint than this share of the leg; the hips twisted further than this from it (degrees)
    double stepThreshold = 0.8;
    double stepReach = 0.55;
    double stepAngleDeg = 50.0;
    double stepTimeMin = 0.22;     // an urgent step (s)
    double stepTimeMax = 0.33;     // a relaxed one
    double legCrossGuard = 0.25;   // a step never lands across the other foot, nor this close to it
    double stepHeight = 0.08;
    double supportMargin = 0.03;   // the centre of mass this far outside the feet makes one step
    // Still for settleDelay (s), a foot further than settleDistance from its place steps back under the body
    double settleDistance = 0.10;
    double settleDelay = 1.0;
    // Going faster than this walks (m/s), slower than gaitStopSpeed stops
    double gaitStartSpeed = 0.5;
    double gaitStopSpeed = 0.29;
    double gaitStepTimeMin = 0.28; // s
    double gaitStepTimeMax = 0.5;
    double gaitSpacing = 0.4;      // walking narrows the stance to this share of footSpacing
    // Faster than this the body runs (m/s), slower than runStopSpeed it walks again
    double runStartSpeed = 2.2;
    double runStopSpeed = 1.8;
    double runStepTime = 0.32;     // s per step: about 3 a second, a light run's
    double runStanceLength = 0.6;  // the body goes at most this far over a foot on the ground (m at reference size)
    double runStepHeight = 0.18;
    double kneeMaxFlexionDeg = 150.0;
    double heelLiftMaxDeg = 35.0;  // a foot rises onto its toes at most this far
    double legStretch = 1.02;      // and the leg stretches at most this much, before the hips come down
    double hipReleaseTau = 0.15;   // the hips' pull down to the feet lets go this slowly (s)
};

struct LegSkeleton {
    double thigh = 0.0, shin = 0.0;
    double ankleHeight = 0.0;      // the ankle above the sole
    double toeForward = 0.0;       // the ball of the foot ahead of the ankle
    double toeHeight = 0.0;        // and above the sole
    double scale = 1.0;
};

struct Step {
    glm::dvec3 from{};
    double fromYaw = 0.0;
    double progress = 0.0;
    double duration = 0.0;
    double length = 0.0;
    bool landed = false; // over where it was going: it only comes down onto it from there, and stays
};

struct Foot {
    glm::dvec3 pos{}; // the floor point under the ankle
    double yaw = 0.0;
    std::optional<Step> step;
    double runLift = 0.0; // running, how high it is off the floor
    glm::dvec3 runFrom{}; // running, where it left the floor
    // Height off the floor: down again by the time the next step may start, so two feet are never in
    // the air together
    double Lift(const LegParams& p, double scale) const;
};

struct FeetInput {
    glm::dvec3 com{};    // the centre of mass
    glm::dvec3 comVel{}; // and its velocity
    double rootYaw = 0.0;
    glm::dvec3 hips[2]{}; // the hip joints [left, right], where the body above the legs has them
    double hipsYaw = 0.0; // the hips' heading
};

class Feet {
public:
    Feet(const LegParams& p, const LegSkeleton& skel, const glm::dvec3& center, double yaw);
    bool Walking() const { return m_gait.has_value() || m_runPhase.has_value(); }
    void Update(const LegParams& p, const LegSkeleton& skel, double dt, const FeetInput& input);

    Foot feet[2]; // [left, right]

private:
    void UpdateGait(const LegParams& p, const LegSkeleton& skel, double dt, const FeetInput& input, const glm::dvec3& v);
    void UpdateBalance(const LegParams& p, const LegSkeleton& skel, double dt, const FeetInput& input);
    void UpdateRun(const LegParams& p, const LegSkeleton& skel, double dt, const FeetInput& input, const glm::dvec3& v);

    std::optional<int> m_gait; // walking: the foot swinging next
    // Running: where the cycle is, from the left foot coming down (0) round to it coming down again (1).
    // The right foot is half a cycle behind.
    std::optional<double> m_runPhase;
    double m_stillTime = 0.0;
};

struct LegPose {
    glm::dvec3 hip{}, knee{}, ankle{}, toes{};
    // Frames (+y along the bone, +z forward / the knee's way): upper leg, lower leg; foot and toes
    // (+y towards the toes, +z up)
    glm::dquat rot[4];
};

// The hip joint heights (lowest, highest) from which this foot can stay planted: the knee bends no
// further than its limit, and stretches no further than heel lift plus legStretch
std::pair<double, double> HipHeightRange(const LegParams& p, const LegSkeleton& skel, const glm::dvec3& hip, const Foot& foot, double lift);

LegPose SolveLeg(const LegParams& p, const LegSkeleton& skel, const glm::dvec3& hip, const Foot& foot, double lift, const glm::dvec3& hipsForward);

} // namespace ik
