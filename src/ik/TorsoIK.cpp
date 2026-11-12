// Port of chasm's VR framework torso yaw (crates/ik/src/torso.rs, MIT, see ArmIK.hpp).
// Not ported: the headset's shoulder tracking and the learned prior.
#include "ik/TorsoIK.hpp"

#include <algorithm>
#include <cmath>

namespace ik {
namespace {

constexpr double Pi = 3.14159265358979323846;
constexpr double Deg = Pi / 180.0;
// Parger settles the both-hands-behind ambiguity with the head: a hand estimate this far from the
// head's yaw is taken to be the flipped one
constexpr double FlipByHead = 120.0 * Deg;
// Hands straight above or below the head joint have no usable horizontal direction
constexpr double MinHorizontal[2] = { 0.05, 0.15 };

double Smoothstep(double edge0, double edge1, double x) {
    const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

glm::dvec3 Flat(const glm::dvec3& v) { return { v.x, 0.0, v.z }; }

glm::dvec3 NormalizeOrZero(const glm::dvec3& v) {
    const double l = glm::length(v);
    return l > 1e-9 ? v / l : glm::dvec3{};
}

glm::dvec3 YawDir(double yaw) { return { std::sin(yaw), 0.0, std::cos(yaw) }; }

} // namespace

double WrapAngle(double a) {
    double w = std::fmod(a + Pi, 2.0 * Pi);
    if (w < 0.0) w += 2.0 * Pi;
    w -= Pi;
    return w <= -Pi ? w + 2.0 * Pi : w;
}

double Smoothing(double dt, double tau) {
    return tau <= 0.0 ? 1.0 : 1.0 - std::exp(-dt / tau);
}

RootYaw::RootYaw(double yaw) : m_yaw(yaw), m_deadZone(DeadZoneStandingDeg * Deg) {}

double RootYaw::Update(double dt, double torsoYaw, bool moving) {
    const double target = (moving ? DeadZoneMovingDeg : DeadZoneStandingDeg) * Deg;
    // VRIK blends between its moving and standing limits over 0.2 s
    m_deadZone += (target - m_deadZone) * Smoothing(dt, 0.2);
    const double off = WrapAngle(torsoYaw - m_yaw);
    if (std::abs(off) > m_deadZone)
        m_yaw = WrapAngle(m_yaw + off - std::copysign(m_deadZone, off));
    return m_yaw;
}

double Follow::Step(double error, double dt, double tau) {
    if (tau <= 0.0) {
        value += error;
        speed = 0.0;
        return value;
    }
    if (dt <= 0.0)
        return value;
    const double w = 2.0 / tau;
    const double e = -error, v = speed;
    const double decay = std::exp(-w * dt);
    const double k = v + w * e;
    const double e1 = (e + k * dt) * decay;
    speed = (v - w * k * dt) * decay;
    value += e1 - e;
    return value;
}

double TorsoYaw::Update(const TorsoParams& p, double dt, double scale, const glm::dvec3& eyes, const glm::dvec3& headJoint,
                        double headYaw, const std::optional<glm::dvec3> hands[2]) {
    double target, tau;
    if (hands[0] && hands[1]) {
        target = headYaw + HandOffset(p, scale, eyes, headJoint, headYaw, *hands[0], *hands[1]);
        tau = p.yawTau;
    } else {
        // One hand alone can't tell a turned torso from a hand held out to the side, so with fewer
        // than two hands the torso just trails the head
        m_previousRaw.reset();
        m_flipped = false;
        target = headYaw;
        tau = p.yawTauHeadOnly;
    }
    m_follow.value = m_yaw;
    m_yaw = WrapAngle(m_follow.Step(WrapAngle(target - m_yaw), dt, tau));
    // Parger keeps the head within 90° of the neck whatever the filter is doing
    const double off = WrapAngle(m_yaw - headYaw);
    if (std::abs(off) > Pi / 2.0) {
        m_yaw = WrapAngle(headYaw + std::copysign(Pi / 2.0, off));
        m_follow.speed = 0.0;
    }
    return m_yaw;
}

double TorsoYaw::HandOffset(const TorsoParams& p, double scale, const glm::dvec3& eyes, const glm::dvec3& headJoint,
                            double headYaw, const glm::dvec3& left, const glm::dvec3& right) {
    const glm::dvec3 f = YawDir(headYaw);
    const glm::dvec3 headRight{ f.z, 0.0, -f.x };
    auto weight = [&](const glm::dvec3& hand) {
        // FRIK: nothing from a hand at the face, less from a hand above the head
        const double nearFace = Smoothstep(p.faceRadius * scale, 2.0 * p.faceRadius * scale, glm::distance(hand, eyes));
        const double above = 1.0 - std::min(std::max(hand.y - eyes.y, 0.0) / (p.aboveHeadRange * scale), 1.0);
        const double horizontal = glm::length(Flat(hand - headJoint)) / scale;
        return nearFace * above * Smoothstep(MinHorizontal[0], MinHorizontal[1], horizontal);
    };
    // FRIK: crossed hands (left of the body's midline to the right of the right hand)
    const double crossing = std::max(glm::dot(left - right, headRight), 0.0);
    const double uncrossed = 1.0 - std::min(crossing / (p.handCrossRange * scale), 1.0);

    // Parger: the sum of the normalized head -> hand directions
    auto dir = [&](const glm::dvec3& hand) { return NormalizeOrZero(Flat(hand - headJoint)); };
    glm::dvec3 sum = dir(left) + dir(right);
    // With the hands level at the sides those directions cancel; the perpendicular of the line
    // between the hands still points forward then, as long as they aren't crossed
    const glm::dvec3 span = Flat(right - left);
    const double lateral = glm::dot(span, headRight);
    const double lineWeight = 2.0 * Smoothstep(0.1 * scale, 0.4 * scale, lateral) * std::max(1.0 - 0.5 * glm::length(sum), 0.0);
    sum += NormalizeOrZero(glm::dvec3{ -span.z, 0.0, span.x }) * lineWeight;

    const double magnitude = glm::length(sum);
    // FRIK zeroes the hand yaw when either hand is unreliable rather than letting the other hand
    // alone steer the torso, so the weights gate the confidence
    const double weights = std::min(weight(left), weight(right));
    const double confidence = Smoothstep(0.2, 0.8, magnitude) * weights * uncrossed;
    if (magnitude < 0.2) {
        m_previousRaw.reset();
        return 0.0;
    }
    const double raw = YawOf(sum);
    // VRArmIK's flip detector: both hands going behind the body swings the sum through zero, so its
    // yaw jumps by about 180° in one step. It only reads hands well out from under the head and away
    // from the face.
    const bool steady = weights > 0.5;
    if (steady && m_previousRaw) {
        double jump = std::fmod(raw - *m_previousRaw, 2.0 * Pi);
        if (jump < 0.0) jump += 2.0 * Pi;
        jump /= Deg;
        if (jump >= p.flipJumpMinDeg && jump <= p.flipJumpMaxDeg)
            m_flipped = !m_flipped;
    }
    m_previousRaw = steady ? std::optional<double>{ raw } : std::nullopt;
    double offset = WrapAngle(raw + (m_flipped ? Pi : 0.0) - headYaw);
    if (std::abs(offset) > FlipByHead) {
        if (steady)
            m_flipped = !m_flipped;
        offset = WrapAngle(offset + Pi);
    }
    const double clamp = p.yawClampDeg * Deg;
    return p.yawBlend * confidence * std::clamp(offset, -clamp, clamp);
}

} // namespace ik
