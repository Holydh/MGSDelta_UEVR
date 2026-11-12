#pragma once
// Torso yaw from the hands, clamped by the head (Parger et al. 2018, VRArmIK, FRIK).
//
// Ported from chasm's VR framework (crates/ik/src/torso.rs and math.rs,
// https://github.com/chasmlol/chasms-vr-framework). MIT License, Copyright (c) 2026 chasmlol: see
// ArmIK.hpp for the full notice.
//
// Units are metres. Positions are in the original's convention: x right, y up, z forward, and a yaw
// of 0 faces +z, positive towards +x.
#include <cmath>
#include <optional>

#include "glm/glm.hpp"

namespace ik {

struct TorsoParams {
    double yawClampDeg = 80.0;  // the hands turn the torso at most this far from the head
    double yawBlend = 0.7;      // and this much of the way
    double yawTau = 0.08;       // s
    double yawTauHeadOnly = 0.5;
    double faceRadius = 0.15;   // a hand within this of the eyes doesn't turn the torso
    double aboveHeadRange = 0.29;
    double handCrossRange = 0.7;
    double flipJumpMinDeg = 150.0;
    double flipJumpMaxDeg = 210.0;
};

// A value carried towards its target as a mass on a critically damped spring: no jolt at either end
struct Follow {
    double value = 0.0;
    double speed = 0.0;
    double Step(double error, double dt, double tau);
};

class TorsoYaw {
public:
    explicit TorsoYaw(double yaw = 0.0) : m_yaw(yaw), m_follow{ yaw, 0.0 } {}

    // eyes and headJoint: the HMD and neck pivot positions, headYaw the head's facing, hands the
    // tracked hand positions [left, right]. Returns the torso's yaw.
    double Update(const TorsoParams& p, double dt, double scale, const glm::dvec3& eyes, const glm::dvec3& headJoint,
                  double headYaw, const std::optional<glm::dvec3> hands[2]);
    double Yaw() const { return m_yaw; }

private:
    double HandOffset(const TorsoParams& p, double scale, const glm::dvec3& eyes, const glm::dvec3& headJoint,
                      double headYaw, const glm::dvec3& left, const glm::dvec3& right);

    double m_yaw;
    Follow m_follow;
    std::optional<double> m_previousRaw; // last hand yaw with a usable magnitude, for the flip detector
    bool m_flipped = false;
};

// The feet and hips' heading: it trails the torso's past a dead zone, narrower while moving (VRIK)
class RootYaw {
public:
    explicit RootYaw(double yaw = 0.0);
    double Update(double dt, double torsoYaw, bool moving);
    double Yaw() const { return m_yaw; }

    static constexpr double DeadZoneStandingDeg = 25.0;
    static constexpr double DeadZoneMovingDeg = 10.0;

private:
    double m_yaw;
    double m_deadZone; // radians
};

double WrapAngle(double a);
// 1 - e^(-dt/tau): a first-order filter's step, frame rate independent
double Smoothing(double dt, double tau);
// Yaw of a direction: 0 along +z, positive towards +x
inline double YawOf(const glm::dvec3& v) { return std::atan2(v.x, v.z); }

} // namespace ik
