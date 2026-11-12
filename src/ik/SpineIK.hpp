#pragma once
// The spine hangs from the head: the neck pivot is where the headset puts it, and the chain below it
// pitches with how far and how the head has dropped (Parger et al. 2018, FRIK, VRIK).
//
// Ported from chasm's VR framework (crates/ik/src/spine.rs and math.rs,
// https://github.com/chasmlol/chasms-vr-framework). MIT License, Copyright (c) 2026 chasmlol: see
// ArmIK.hpp for the full notice.
//
// Units are metres. Positions are in the original's convention: x right, y up, z forward, and a yaw
// of 0 faces +z, positive towards +x.
#include <numbers>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace ik {

struct SpineParams {
    // Spine pitch per relative head drop, plus this share of the head's pitch (Parger 135.3° and
    // 0.333, FRIK 105.3° and 0.1)
    double bendDeg = 120.0;
    double bendHeadPitch = 0.2;
    // The drop bends the spine in full only with the gaze this far down, and not at all below the
    // start: dropping with a level gaze is a crouch (knees), dropping while looking down a bend
    double bendGazeStartDeg = 10.0;
    double bendGazeFullDeg = 40.0;
    double pelvisBack = 0.5;    // in a crouch the hips go back by this share of the drop (VRIK)
    double minHeadHeight = 0.8; // the head is seen no lower than this (m at reference size)
    double tau = 0.05;          // pitch smoothing (s)
};

// The neck leans forward this much from the chest's up, upright
constexpr double NeckLean = 10.0 * std::numbers::pi / 180.0;

// Hips -> spine -> chest -> upper chest -> neck, and the neck to the head joint
struct SpineSkeleton {
    double spine[4];
    double neck;
    double scale;
};

// Spine pitch (radians, positive bends forward) for a level eye height eyesY over the floor, a
// standing one h0 and a head pitch (radians, positive looking down)
double TargetPitch(const SpineParams& p, const SpineSkeleton& skel, double eyesY, double h0, double headPitch);

struct ChainInput {
    glm::dvec3 headJoint;
    glm::dquat headRot; // +y up through the head, +z the way the face looks
    double torsoYaw;
    double rootYaw;
    double pitch;
};

// Hips, spine, chest, upper chest, neck, head: positions, and frames whose +y runs up the segment to
// the next joint and +z forward
struct Chain {
    glm::dvec3 pos[6];
    glm::dquat rot[6];
};

Chain BuildChain(const SpineSkeleton& skel, const ChainInput& input);

// A rotation whose +y is y and whose +z is as near zHint as it can be (fallbackZ when zHint is
// along y); x = y × z
glm::dquat FrameYZ(const glm::dvec3& y, const glm::dvec3& zHint, const glm::dvec3& fallbackZ);

} // namespace ik
