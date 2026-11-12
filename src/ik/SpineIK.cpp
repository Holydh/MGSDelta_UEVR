// Port of chasm's VR framework spine (crates/ik/src/spine.rs, MIT, see ArmIK.hpp).
// Not ported: the seated (riding) spine.
#include "ik/SpineIK.hpp"

#include <algorithm>
#include <cmath>

#include "ik/TorsoIK.hpp"

namespace ik {
namespace {

constexpr double Deg = std::numbers::pi / 180.0;
// Share of the spine pitch each segment takes, hips up to the neck: the upper back curls a little more
constexpr double PitchShare[4] = { 0.85, 0.95, 1.05, 1.15 };
// Share of the torso's twist over the root each joint takes, hips up to the upper chest
constexpr double TwistShare[4] = { 0.3, 0.55, 0.8, 1.0 };
// The neck bends halfway between the upper chest and the head
constexpr double NeckFollow = 0.5;

const glm::dvec3 Up{ 0, 1, 0 };
const glm::dvec3 Right{ 1, 0, 0 };

double Smoothstep(double edge0, double edge1, double x) {
    const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

glm::dvec3 NormalizeOr(const glm::dvec3& v, const glm::dvec3& fallback) {
    const double l = glm::length(v);
    return l > 1e-9 ? v / l : fallback;
}

glm::dquat YawRot(double yaw) { return glm::angleAxis(yaw, Up); }

// +y up the segment, +x as near the body's right as it can be: built from the right, as bent right
// over, the spine runs the way the body faces
glm::dquat SpineFrame(const glm::dvec3& up, const glm::dvec3& right) { return FrameYZ(up, glm::cross(right, up), Up); }

// The neck's way up from its base, in a frame whose +y is up the spine and +z forward
glm::dvec3 NeckUp(const glm::dquat& frame) { return frame * glm::dvec3{ 0.0, std::cos(NeckLean), std::sin(NeckLean) }; }

} // namespace

glm::dquat FrameYZ(const glm::dvec3& y, const glm::dvec3& zHint, const glm::dvec3& fallbackZ) {
    const glm::dvec3 yn = NormalizeOr(y, Up);
    auto reject = [&yn](const glm::dvec3& v) { return v - yn * glm::dot(v, yn); };
    glm::dvec3 z = reject(zHint);
    if (glm::length(z) < 1e-6)
        z = reject(fallbackZ);
    if (glm::length(z) < 1e-6)
        z = reject(std::abs(yn.x) < 0.9 ? Right : Up);
    z = glm::normalize(z);
    return glm::normalize(glm::quat_cast(glm::dmat3(glm::cross(yn, z), yn, z)));
}

double TargetPitch(const SpineParams& p, const SpineSkeleton& skel, double eyesY, double h0, double headPitch) {
    const double h = std::max(eyesY, p.minHeadHeight * skel.scale);
    const double drop = std::clamp((h0 - h) / h0, 0.0, 1.0);
    // Crouch versus bend: the drop bends the spine only as far as the gaze is down
    const double gaze = Smoothstep(p.bendGazeStartDeg, p.bendGazeFullDeg, headPitch / Deg);
    const double bend = drop * p.bendDeg * Deg * gaze + p.bendHeadPitch * headPitch;
    // VRIK moves the body back as the head drops; with the head fixed that leans the spine
    const double crouch = std::asin(std::min(p.pelvisBack * drop, 1.0));
    return std::clamp(bend + crouch * (1.0 - gaze), MinPitch, MaxPitch);
}

Chain BuildChain(const SpineSkeleton& skel, const ChainInput& input) {
    const double twist = WrapAngle(input.torsoYaw - input.rootYaw);
    double yaw[4];
    glm::dquat frames[4];
    glm::dvec3 dirs[4];
    for (int i = 0; i < 4; i++)
    {
        yaw[i] = input.rootYaw + twist * TwistShare[i];
        // Each segment pitched forward about its own right
        frames[i] = YawRot(yaw[i]) * glm::angleAxis(input.pitch * (input.lying ? 1.0 : PitchShare[i]), Right);
        dirs[i] = frames[i] * Up;
    }
    const glm::dvec3 neckDir = NormalizeOr(glm::mix(NeckUp(frames[3]), NeckUp(input.headRot), NeckFollow), dirs[3]);

    Chain chain;
    for (int i = 0; i < 4; i++)
        chain.yaw[i] = yaw[i];
    chain.pos[5] = input.headJoint;
    chain.pos[4] = input.headJoint - neckDir * skel.neck;
    for (int i = 3; i >= 0; i--)
        chain.pos[i] = chain.pos[i + 1] - dirs[i] * skel.spine[i];
    for (int i = 0; i < 4; i++)
        chain.rot[i] = SpineFrame(chain.pos[i + 1] - chain.pos[i], YawRot(yaw[i]) * Right);
    // The neck's right: between the chest's and the head's, as far as it follows the head
    const glm::dvec3 chestRight = YawRot(yaw[3]) * Right;
    chain.rot[4] = SpineFrame(neckDir, NormalizeOr(glm::mix(chestRight, input.headRot * Right, NeckFollow), chestRight));
    chain.rot[5] = input.headRot;
    return chain;
}

void ShiftHips(Chain& chain, double dy) {
    constexpr double Share[4] = { 1.0, 2.0 / 3.0, 1.0 / 3.0, 0.0 };
    for (int i = 0; i < 4; i++)
        chain.pos[i].y -= dy * Share[i];
    for (int i = 0; i < 4; i++)
        chain.rot[i] = SpineFrame(chain.pos[i + 1] - chain.pos[i], YawRot(chain.yaw[i]) * Right);
}

} // namespace ik
