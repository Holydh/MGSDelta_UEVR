// Port of chasm's VR framework feet and legs (crates/ik/src/legs.rs, MIT, see ArmIK.hpp).
// Not ported: seated feet, kicks.
#include "ik/LegsIK.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

#include "ik/ArmIK.hpp"
#include "ik/SpineIK.hpp"
#include "ik/TorsoIK.hpp"

namespace ik {
namespace {

constexpr double Pi = std::numbers::pi;
constexpr double Deg = Pi / 180.0;
// The knee keeps a sliver of bend so its direction stays defined
constexpr double KneeMinFlexion = 3.0 * Deg;
// The last stretch to a straight leg, a share of its reach: the knee straightens at a steady rate
// over it while the leg lengthens the little it takes (a knee near straight would pop with every bob)
constexpr double LegEase = 0.01;
// A stepping foot rises (and at the end comes down) over this share of the time it's in the air
constexpr double StepLiftOff = 0.15;
// A new step may start once the other foot is this far through its own (VRIK)
constexpr double Overlap = 0.8;
// VRIK biases the tie-break towards the left foot
constexpr double LeftBias[2] = { 0.9, 1.0 };
// Below this centre-of-mass speed the body counts as still, for settling steps (m/s)
constexpr double StillSpeed = 0.1;
// A foot on the floor bears weight from its heel, this far behind the ankle, to its toes, this far
// ahead of the ball (m at reference size)
constexpr double HeelBack = 0.06;
constexpr double ToesAhead = 0.05;
// Seconds ahead the centre of mass is predicted for balance steps
constexpr double ComAhead = 0.15;

const glm::dvec3 Up{ 0, 1, 0 };

glm::dvec3 Flat(const glm::dvec3& v) { return { v.x, 0.0, v.z }; }
glm::dvec3 YawDir(double yaw) { return { std::sin(yaw), 0.0, std::cos(yaw) }; }
glm::dvec3 RightOf(double yaw) {
    const glm::dvec3 f = YawDir(yaw);
    return { f.z, 0.0, -f.x };
}

glm::dvec3 NormalizeOr(const glm::dvec3& v, const glm::dvec3& fallback) {
    const double l = glm::length(v);
    return l > 1e-9 ? v / l : fallback;
}

glm::dvec3 ClampLength(const glm::dvec3& v, double max) {
    const double l = glm::length(v);
    return l > max ? v * (max / l) : v;
}

// A foot's spot beside center: left (0) or right (1) of it, spacing apart
glm::dvec3 Spot(const LegSkeleton& skel, const glm::dvec3& center, double yaw, int side, double spacing) {
    const double sign = side == 0 ? -1.0 : 1.0;
    return Flat(center) + RightOf(yaw) * (sign * spacing * 0.5 * skel.scale);
}

// VRIK's leg-crossing guard: a step never lands across the other foot, nor within legCrossGuard of
// it; such targets are pushed out sideways
glm::dvec3 CrossGuard(const LegParams& p, const LegSkeleton& skel, const glm::dvec3& target, const glm::dvec3& other, double yaw, int side) {
    const glm::dvec3 outward = RightOf(yaw) * (side == 0 ? -1.0 : 1.0);
    const glm::dvec3 rel = Flat(target - other);
    const double lateral = std::max(glm::dot(rel, outward), 0.0);
    const glm::dvec3 along = rel - outward * glm::dot(rel, outward);
    const double guard = p.legCrossGuard * skel.scale;
    const double needed = std::sqrt(std::max(guard * guard - glm::dot(along, along), 0.0));
    return Flat(other) + along + outward * std::max(lateral, needed);
}

// How far p is outside the feet's base of support (the outline round both feet, heels to toes),
// horizontally; 0 inside it
double OutsideSupport(const LegSkeleton& skel, const Foot (&feet)[2], const glm::dvec3& point) {
    glm::dvec3 corners[4];
    for (int i = 0; i < 2; i++)
    {
        const glm::dvec3 fwd = YawDir(feet[i].yaw);
        corners[2 * i] = Flat(feet[i].pos) - fwd * (HeelBack * skel.scale);
        corners[2 * i + 1] = Flat(feet[i].pos) + fwd * (skel.toeForward + ToesAhead * skel.scale);
    }
    const glm::dvec3 p = Flat(point);
    // The convex outline of the four corners (gift wrapping), then the distance to it
    int start = 0;
    for (int k = 1; k < 4; k++)
        if (corners[k].x < corners[start].x)
            start = k;
    int hull[4] = { start, 0, 0, 0 };
    int n = 1;
    for (;;)
    {
        const int cur = hull[n - 1];
        int next = (cur + 1) % 4;
        for (int k = 0; k < 4; k++)
        {
            const glm::dvec3 a = corners[next] - corners[cur], b = corners[k] - corners[cur];
            // A corner to the right of the edge so far takes its place: the outline turns left
            if (a.x * b.z - a.z * b.x > 0.0)
                next = k;
        }
        if (next == start || n == 4)
            break;
        hull[n++] = next;
    }
    bool inside = n >= 3;
    double nearest = std::numeric_limits<double>::max();
    for (int k = 0; k < n; k++)
    {
        const glm::dvec3 a = corners[hull[k]], b = corners[hull[(k + 1) % n]];
        const glm::dvec3 ab = b - a;
        if (ab.x * (p - a).z - ab.z * (p - a).x > 0.0)
            inside = false;
        const double t = std::clamp(glm::dot(p - a, ab) / std::max(glm::dot(ab, ab), 1e-8), 0.0, 1.0);
        nearest = std::min(nearest, glm::distance(p, a + ab * t));
    }
    return inside ? 0.0 : nearest;
}

// Moves a stepping foot along its step towards target, which re-tracks every frame until the foot is
// over it: it travels while in the air, then comes straight down there and stays. True when the step
// is over.
bool Advance(Foot& foot, const glm::dvec3& target, double yaw) {
    if (!foot.step)
        return false;
    Step& step = *foot.step;
    if (!step.landed)
    {
        const double t = std::min(step.progress / Overlap, 1.0);
        // VRIK's InOutSine, between the foot coming up off the floor and going down onto it
        const double travel = std::clamp((t - StepLiftOff) / (1.0 - 2.0 * StepLiftOff), 0.0, 1.0);
        const double ease = 0.5 - 0.5 * std::cos(Pi * travel);
        foot.pos = glm::mix(step.from, target, ease);
        foot.yaw = WrapAngle(step.fromYaw + WrapAngle(yaw - step.fromYaw) * ease);
        step.landed = travel >= 1.0;
    }
    if (step.progress >= 1.0)
    {
        foot.step.reset();
        return true;
    }
    return false;
}

// The ankle and the ball of the foot for a foot lift off the floor, the heel raised heelLift radians
// about the ball
std::pair<glm::dvec3, glm::dvec3> AnkleAt(const LegSkeleton& skel, const Foot& foot, double lift, double heelLift) {
    const glm::dvec3 fwd = YawDir(foot.yaw);
    const glm::dvec3 ball = foot.pos + fwd * skel.toeForward + Up * (skel.toeHeight + lift);
    const glm::dvec3 flatAnkle = foot.pos + Up * (skel.ankleHeight + lift);
    const glm::dquat pitch = glm::angleAxis(heelLift, RightOf(foot.yaw));
    return { ball + pitch * (flatAnkle - ball), ball };
}

} // namespace

double Foot::Lift(const LegParams& p, double scale) const {
    if (!step)
        return runLift;
    const double height = p.stepHeight * scale * std::clamp(step->length / (0.25 * scale), 0.5, 1.0);
    // Rising from rest and settling to rest: no jolt at either end
    return 0.5 * (1.0 - std::cos(2.0 * Pi * std::min(step->progress / Overlap, 1.0))) * height;
}

Feet::Feet(const LegParams& p, const LegSkeleton& skel, const glm::dvec3& center, double yaw) {
    for (int i = 0; i < 2; i++)
        feet[i] = { Spot(skel, Flat(center), yaw, i, p.footSpacing), yaw, std::nullopt };
}

void Feet::Update(const LegParams& p, const LegSkeleton& skel, double dt, const FeetInput& input) {
    // The feet are planted in the world, which the body goes through by the stick or on its own feet:
    // either way it walks, the gait setting the feet down a stride apart, ahead of it
    const glm::dvec3 walk = Flat(input.comVel);
    const double speed = glm::length(walk);
    // Running has a cycle of its own: walking, a foot stays down as long as the other swings, and at
    // running speeds the legs would reach a whole stride apart
    if (speed >= (m_runPhase ? p.runStopSpeed : p.runStartSpeed))
    {
        if (!m_runPhase)
        {
            // The foot further ahead comes down first
            const double ahead0 = glm::dot(feet[0].pos - feet[1].pos, walk);
            m_runPhase = ahead0 >= 0.0 ? 0.0 : 0.5;
            m_gait.reset();
            for (Foot& f : feet)
            {
                f.step.reset();
                f.runFrom = f.pos;
            }
        }
        UpdateRun(p, skel, dt, input, walk);
        return;
    }
    if (m_runPhase)
    {
        // Walking again: the feet come down where they are, the one further behind swinging first
        m_runPhase.reset();
        for (Foot& f : feet)
            f.runLift = 0.0;
        auto behind = [&](const Foot& f) { return -glm::dot(f.pos - Flat(input.com), walk); };
        m_gait = behind(feet[1]) > behind(feet[0]) ? 1 : 0;
    }
    // A balance step in progress finishes before the gait starts
    const bool stepping = feet[0].step || feet[1].step;
    if (!m_gait && !stepping && speed >= p.gaitStartSpeed)
    {
        // Start with the foot furthest behind along the travel direction
        auto behind = [&](const Foot& f) { return -glm::dot(f.pos - Flat(input.com), walk); };
        m_gait = behind(feet[1]) > behind(feet[0]) ? 1 : 0;
    }
    if (m_gait)
        UpdateGait(p, skel, dt, input, walk);
    else
        UpdateBalance(p, skel, dt, input);
}

void Feet::UpdateGait(const LegParams& p, const LegSkeleton& skel, double dt, const FeetInput& input, const glm::dvec3& v) {
    // FRIK's phase generator: feet swing alternately, each landing half a step ahead of the body so it
    // slides back through the stance to half a step behind
    const int swing = *m_gait;
    const double speed = glm::length(v);
    // FRIK: step time = clamp(cos(speed / 2 m/s))
    const double duration = std::clamp(std::cos(speed / 2.0), p.gaitStepTimeMin, p.gaitStepTimeMax);
    const glm::dvec3 other = feet[1 - swing].pos;
    Foot& foot = feet[swing];
    if (!foot.step)
        foot.step = Step{ foot.pos, foot.yaw, 0.0, duration, speed * duration, false };
    foot.step->duration = duration;
    foot.step->length = speed * duration;
    foot.step->progress += dt / duration;
    const glm::dvec3 ahead = Flat(input.com) + v * (duration * 0.5);
    const glm::dvec3 target = CrossGuard(p, skel, Spot(skel, ahead, input.rootYaw, swing, p.footSpacing * p.gaitSpacing), other, input.rootYaw, swing);
    if (Advance(foot, target, input.rootYaw))
    {
        if (speed >= p.gaitStopSpeed)
            m_gait = 1 - swing;
        else
        {
            m_gait.reset();
            // Stopping, the other foot closes up to a standing stance at once (a walk's last step)
            m_stillTime = std::max(m_stillTime, p.settleDelay);
        }
    }
}

void Feet::UpdateRun(const LegParams& p, const LegSkeleton& skel, double dt, const FeetInput& input, const glm::dvec3& v) {
    // Each foot is down while the body goes runStanceLength over it (at most half the cycle: then it's
    // a walk), and in the air the rest of the cycle, both feet at once part of it. It comes down half
    // its stance ahead of where the body will be, so the body goes over it from front to back.
    const double cycle = 2.0 * p.runStepTime;
    const double speed = glm::length(v);
    const double duty = std::min(0.5, p.runStanceLength * skel.scale / std::max(speed * cycle, 1e-6));
    const double previous = *m_runPhase;
    m_runPhase = std::fmod(previous + dt / cycle, 1.0);
    for (int i = 0; i < 2; i++)
    {
        Foot& foot = feet[i];
        const double offset = i == 0 ? 0.0 : 0.5;
        const double before = std::fmod(previous + offset, 1.0);
        const double now = std::fmod(*m_runPhase + offset, 1.0);
        if (now < duty)
        {
            foot.runLift = 0.0;
            continue;
        }
        if (before < duty)
            foot.runFrom = foot.pos; // it's just left the floor
        const double t = (now - duty) / (1.0 - duty);
        const double untilDown = (1.0 - now) * cycle;
        const glm::dvec3 land = Spot(skel, Flat(input.com) + v * (untilDown + 0.5 * duty * cycle), input.rootYaw, i, p.footSpacing * p.gaitSpacing);
        const double ease = 0.5 - 0.5 * std::cos(Pi * t);
        foot.pos = glm::mix(foot.runFrom, land, ease);
        foot.yaw = input.rootYaw;
        foot.runLift = p.runStepHeight * skel.scale * std::sin(Pi * t);
    }
}

void Feet::UpdateBalance(const LegParams& p, const LegSkeleton& skel, double dt, const FeetInput& input) {
    // A foot steps only when it must: the centre of mass, a little ahead by its velocity, leaves the
    // outline round both feet; the leg can't comfortably reach; the hips have twisted too far from it;
    // or it's far from its place whatever the rest. Standing still a while, a foot badly out of place
    // steps back under the body.
    const double s = skel.scale;
    const glm::dvec3 com = input.com + ClampLength(input.comVel, 0.4) * ComAhead;
    const glm::dvec3 ground = Flat(com);
    if (glm::length(Flat(input.comVel)) < StillSpeed)
        m_stillTime += dt;
    else
        m_stillTime = 0.0;
    glm::dvec3 targets[2];
    for (int i = 0; i < 2; i++)
        targets[i] = CrossGuard(p, skel, Spot(skel, ground, input.rootYaw, i, p.footSpacing), feet[1 - i].pos, input.rootYaw, i);
    for (int i = 0; i < 2; i++)
    {
        if (feet[i].step)
        {
            feet[i].step->progress += dt / feet[i].step->duration;
            Advance(feet[i], targets[i], input.rootYaw);
        }
    }

    for (const Foot& f : feet)
        if (f.step && f.step->progress < Overlap)
            return;
    const double margin = p.supportMargin * s;
    const double outside = OutsideSupport(skel, feet, ground) - margin;
    const bool settled = m_stillTime >= p.settleDelay;
    int best = -1;
    double bestNeed = 0.0;
    for (int i = 0; i < 2; i++)
    {
        const Foot& foot = feet[i];
        if (foot.step)
            continue;
        const double dist = glm::length(Flat(targets[i] - foot.pos));
        const double limit = p.stepThreshold * s * LeftBias[i];
        const bool turned = std::abs(WrapAngle(input.hipsYaw - foot.yaw)) > p.stepAngleDeg * Deg;
        const double reach = (skel.thigh + skel.shin) * p.stepReach;
        const double overreach = glm::length(Flat(input.hips[i] - foot.pos)) - reach;
        double need;
        if (outside > 0.0 || overreach > 0.0 || dist > limit || turned)
            need = 1.0 + std::max(outside / margin, 0.0) + std::max(overreach / (0.1 * s), 0.0) + dist / limit;
        else if (settled && dist > p.settleDistance * s)
            need = dist / limit;
        else
            continue;
        if (best < 0 || need > bestNeed)
        {
            best = i;
            bestNeed = need;
        }
    }
    if (best < 0)
        return;
    Foot& foot = feet[best];
    const double length = glm::length(Flat(targets[best] - foot.pos));
    const double turn = std::abs(WrapAngle(input.rootYaw - foot.yaw));
    // A step of a centimetre or two, turning a few degrees, is no step: the foot stays
    if (length < 0.03 * s && turn < 0.2)
        return;
    // Urgent steps are quicker
    const double urgency = std::clamp(bestNeed - 1.0, 0.0, 1.0);
    const double duration = p.stepTimeMax + (p.stepTimeMin - p.stepTimeMax) * urgency;
    foot.step = Step{ foot.pos, foot.yaw, 0.0, duration, length, false };
    m_stillTime = 0.0;
}

std::pair<double, double> HipHeightRange(const LegParams& p, const LegSkeleton& skel, const glm::dvec3& hip, const Foot& foot, double lift) {
    const double a = skel.thigh, b = skel.shin;
    const double fold = p.kneeMaxFlexionDeg * Deg;
    const double closest = std::sqrt(a * a + b * b + 2.0 * a * b * std::cos(fold));
    const glm::dvec3 flatAnkle = AnkleAt(skel, foot, lift, 0.0).first;
    double horizontal = glm::length(Flat(hip - flatAnkle));
    const double low = flatAnkle.y + std::sqrt(std::max(closest * closest - horizontal * horizontal, 0.0));
    const glm::dvec3 raised = AnkleAt(skel, foot, lift, p.heelLiftMaxDeg * Deg).first;
    const double reach = (a + b) * p.legStretch;
    horizontal = glm::length(Flat(hip - raised));
    const double high = raised.y + std::sqrt(std::max(reach * reach - horizontal * horizontal, 0.0));
    return { low, std::max(high, low) };
}

LegPose SolveLeg(const LegParams& p, const LegSkeleton& skel, const glm::dvec3& hip, const Foot& foot, double lift, const glm::dvec3& hipsForward) {
    const double a = skel.thigh, b = skel.shin;
    const double reach = std::sqrt(a * a + b * b + 2.0 * a * b * std::cos(KneeMinFlexion));
    // Rise onto the toes only as far as needed to reach the ankle
    const double maxLift = p.heelLiftMaxDeg * Deg;
    double heel = 0.0;
    if (glm::distance(AnkleAt(skel, foot, lift, 0.0).first, hip) > reach)
    {
        double lo = 0.0, hi = maxLift;
        for (int i = 0; i < 12; i++)
        {
            const double mid = 0.5 * (lo + hi);
            if (glm::distance(AnkleAt(skel, foot, lift, mid).first, hip) > reach)
                lo = mid;
            else
                hi = mid;
        }
        heel = hi;
    }
    const auto [ankle, toes] = AnkleAt(skel, foot, lift, heel);
    const glm::dvec3 fwd = YawDir(foot.yaw);
    // VRIK bendToTargetWeight 0.5: the knee points between the foot and the hips
    const glm::dvec3 kneeDir = NormalizeOr(fwd + NormalizeOr(Flat(hipsForward), fwd), fwd);
    const double flex[2] = { KneeMinFlexion, p.kneeMaxFlexionDeg * Deg };
    // Feet stay planted: the leg stretches as far as it must (the hips have already come down as far
    // as the spine allows), and eases into its last stretch to straight
    const double d = glm::distance(hip, ankle);
    const double bend = EasedReach(d, reach, LegEase * reach);
    const double k = bend > 1e-6 ? std::max(d / bend, 1.0) : 1.0;
    const double lengths[2] = { a * k, b * k };
    const TwoBoneResult solved = TwoBone(hip, ankle, lengths, kneeDir, flex, std::numeric_limits<double>::max(), -Up);
    const glm::dvec3 thighDir = NormalizeOr(solved.elbow - hip, -Up);
    const glm::dquat pitch = glm::angleAxis(heel, RightOf(foot.yaw));
    LegPose pose;
    pose.hip = hip;
    pose.knee = solved.elbow;
    pose.ankle = ankle;
    pose.toes = toes;
    pose.rot[0] = FrameYZ(thighDir, kneeDir, fwd);
    pose.rot[1] = FrameYZ(solved.forearm, kneeDir, fwd);
    pose.rot[2] = FrameYZ(pitch * fwd, pitch * Up, Up);
    pose.rot[3] = FrameYZ(fwd, Up, Up);
    return pose;
}

} // namespace ik
