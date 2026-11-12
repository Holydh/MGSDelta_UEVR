#pragma once
// Arm IK: the elbow swivel and an analytic two-bone solve with flexion clamps.
//
// Ported from chasm's VR framework (crates/ik/src/arm.rs, https://github.com/chasmlol/chasms-vr-framework),
// which builds on VRIK, FRIK, VRArmIK and Parger et al. 2018.
//
// MIT License
// Copyright (c) 2026 chasmlol
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions: The above copyright notice and this
// permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Units are metres. Directions are given by the chest frame, so the solver doesn't depend on the
// engine's axes: chest-local coordinates are (x right, y up, z forward), as in the original.
#include <optional>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace ik {

enum class Side { Left, Right };

// The upper chest: position and orthonormal axes
struct ChestFrame {
    glm::dvec3 pos{};
    glm::dvec3 right{ 0, 1, 0 }, up{ 0, 0, 1 }, forward{ 1, 0, 0 };

    glm::dvec3 ToLocal(const glm::dvec3& v) const { return { glm::dot(v, right), glm::dot(v, up), glm::dot(v, forward) }; }
    glm::dvec3 ToWorld(const glm::dvec3& l) const { return right * l.x + up * l.y + forward * l.z; }
};

// The hand target: the wrist position and the hand's axes (fingers, thumb side)
struct HandTarget {
    glm::dvec3 wrist{};
    glm::dvec3 fingers{};
    glm::dvec3 thumb{};
};

struct ArmParams {
    double elbowMinFlexionDeg = 0.0;
    double elbowMaxFlexionDeg = 145.0;
    double swivelTau = 0.06;     // elbow swivel smoothing (s); the hand is never smoothed
    double armStretch = 1.0;     // bones never stretch: Snake's mesh would stretch with them
    double trackingLoss = 2.25;  // a hand further than this many arm lengths is tracking loss
    // How far the shoulder goes towards a hand past the arm's reach (m at reference size): forward or
    // across the chest, back, up, down, out
    double shoulderReach[5] = { 0.15, 0.05, 0.10, 0.04, 0.03 };
    double armEase = 0.02;       // over this share of the arm's reach either side of full, the shoulder takes over
};

struct ArmSkeleton {
    double upperArm = 0.0;
    double lowerArm = 0.0;
    double scale = 1.0;      // against the reference body the constants are tuned for
    double shoulderUp = 0.0; // shoulder line above the upper chest joint
    double ArmLength() const { return upperArm + lowerArm; }
};

struct ArmState {
    std::optional<glm::dvec3> pole;   // smoothed elbow direction, chest-local
    std::optional<glm::dvec3> upper;  // the upper arm's direction last frame, chest-local with +X outward
    std::optional<glm::dvec2> girdle; // the shoulder girdle's lift and swing forward (radians), smoothed
};

struct ArmPose {
    glm::dvec3 elbow{};
    glm::dvec3 wrist{};   // the hand target, or the end of the straight arm when out of reach
    glm::dvec3 upperDir{};
    glm::dvec3 forearmDir{};
    glm::dvec3 fold{};    // square to the upper arm, the way the forearm folds
    bool reached = false;
};

// Solves one arm. shoulder: the shoulder joint, which doesn't move. Returns nothing on tracking loss.
// Where the shoulder joint goes, about the clavicle's root from where it rests: lifted and swung
// with the way the upper arm pointed last frame (the shoulder girdle), and towards a hand near or
// past the arm's reach. The clavicle keeps its length.
glm::dvec3 PlaceShoulder(const ArmParams& p, const ArmSkeleton& skel, double dt, ArmState& state, Side side,
                         const ChestFrame& chest, const glm::dvec3& clavicleRoot, const glm::dvec3& restShoulder, const glm::dvec3& wrist);

std::optional<ArmPose> SolveArm(const ArmParams& p, const ArmSkeleton& skel, double dt, ArmState& state, Side side,
                                const ChestFrame& chest, const glm::dvec3& shoulder, const HandTarget& target);

} // namespace ik
