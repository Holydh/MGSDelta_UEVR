// Port of chasm's VR framework arm solver (crates/ik/src/arm.rs, MIT, see ArmIK.hpp).
// Not ported: the learned prior and tracked elbows.
#include "ik/ArmIK.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "glm/gtx/quaternion.hpp"

namespace ik {
namespace {

constexpr double Pi = 3.14159265358979323846;
constexpr double Deg = Pi / 180.0;

// Parger's fixed elbow direction for hands near the shoulder's vertical axis or behind it, in
// shoulder space for the right arm (+X outward, +Y up, +Z forward)
constexpr glm::dvec3 PargerFixedElbow{ 0.133, -0.443, -0.886 };
// Parger blends towards it within 0.5 m of the shoulder's vertical axis, and over the first 0.1 m behind the shoulder
constexpr double SingularRadius = 0.5;
constexpr double SingularBehind = 0.1;
// Comfortable ranges (degrees) of the wrist and forearm; beyond them the swivel pays a quadratic cost
constexpr double WristFlex = 50.0;
constexpr double WristUlnar = 30.0;
constexpr double WristRadial = 15.0;
constexpr double ForearmTwist = 70.0;
// Cost weights, per radian² past those ranges, against 1 for swinging the elbow 90° away from the position model
constexpr double WWrist = 8.0;
constexpr double WTwist = 4.0;
constexpr double WPrevious = 0.3;
constexpr double WTorso = 20.0;
// An elbow tucked further in towards the body's middle than both the shoulder and the wrist pays this per 10 cm past them
constexpr double WInward = 2.0;
// The swivel search runs from just above outward round to a little inward under the arm
constexpr double SwivelRangeDeg[2] = { 30.0, 215.0 };
constexpr double SwivelStepDeg = 10.0;
// The elbow is kept out of a box around the ribcage (upper chest space, metres at reference size):
// half width, back, front, bottom
constexpr double TorsoBox[4] = { 0.14, -0.12, 0.11, -0.55 };

glm::dvec3 RejectFrom(const glm::dvec3& v, const glm::dvec3& axis) {
    const double l2 = glm::dot(axis, axis);
    return l2 > 0.0 ? v - axis * (glm::dot(v, axis) / l2) : v;
}

std::optional<glm::dvec3> TryNormalize(const glm::dvec3& v) {
    const double l = glm::length(v);
    if (!(l > 1e-9) || !std::isfinite(l)) return std::nullopt;
    return v / l;
}

glm::dvec3 NormalizeOr(const glm::dvec3& v, const glm::dvec3& fallback) { return TryNormalize(v).value_or(fallback); }

// Any unit vector square to the unit vector v (glam's any_orthonormal_vector)
glm::dvec3 AnyOrthonormal(const glm::dvec3& v) {
    const double sign = std::copysign(1.0, v.z);
    const double a = -1.0 / (sign + v.z);
    const double b = v.x * v.y * a;
    return { b, sign + v.y * v.y * a, -v.y };
}

// Frame-rate independent smoothing factor
double Smoothing(double dt, double tau) { return tau <= 0.0 ? 1.0 : 1.0 - std::exp(-dt / tau); }

double Smoothstep(double edge0, double edge1, double x) {
    const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

double WrapAngle(double a) {
    double w = std::fmod(a + Pi, 2.0 * Pi);
    if (w < 0.0) w += 2.0 * Pi;
    w -= Pi;
    return w <= -Pi ? w + 2.0 * Pi : w;
}

// Signed angle from a to b around axis (a and b are projected onto the plane normal to axis)
double SignedAngle(const glm::dvec3& a, const glm::dvec3& b, const glm::dvec3& axis) {
    const glm::dvec3 n = NormalizeOr(axis, {});
    const glm::dvec3 pa = RejectFrom(a, n), pb = RejectFrom(b, n);
    return std::atan2(glm::dot(glm::cross(pa, pb), n), glm::dot(pa, pb));
}

glm::dquat RotationArc(const glm::dvec3& from, const glm::dvec3& to) {
    return glm::rotation(NormalizeOr(from, { 0, 0, 1 }), NormalizeOr(to, { 0, 0, 1 }));
}

// Clamp with a smooth knee of width knee inside each limit (VRArmIK soft-clamps the swivel)
double SoftClamp(double x, double lo, double hi, double knee) {
    auto soft = [knee](double over) { return knee * (1.0 - std::exp(-over / knee)); };
    if (x > hi - knee) return hi - knee + soft(x - (hi - knee));
    if (x < lo + knee) return lo + knee - soft(lo + knee - x);
    return x;
}

// Swivel frame around the shoulder -> wrist axis: elbow direction e(α) = u cos α + o sin α, so α = 0
// points up, 90° outward and 180° down
struct SwivelFrame {
    glm::dvec3 u{}, o{};

    SwivelFrame(const glm::dvec3& axis, const glm::dvec3& up, const glm::dvec3& forward, const glm::dvec3& outward) {
        // When the arm hangs (or points straight up) "up" around it is undefined; leaning it forward
        // then makes "down" point back for a hanging arm, where elbows go.
        const double lean = -glm::dot(axis, up) * 0.2;
        u = TryNormalize(RejectFrom(up, axis) + RejectFrom(forward, axis) * lean).value_or(AnyOrthonormal(axis));
        // Outward stays outward even with the hand behind the body; with the arm itself pointing
        // outward, backward takes over.
        const glm::dvec3 out = RejectFrom(RejectFrom(outward, axis) - RejectFrom(forward, axis) * 0.3, u);
        if (auto n = TryNormalize(out)) {
            o = *n;
        } else {
            const glm::dvec3 c = glm::cross(u, axis);
            o = glm::dot(c, outward) >= 0.0 ? c : -c;
        }
    }

    glm::dvec3 Dir(double alpha) const { return u * std::cos(alpha) + o * std::sin(alpha); }
    double Angle(const glm::dvec3& e) const { return std::atan2(glm::dot(e, o), glm::dot(e, u)); }
};

} // namespace

TwoBoneResult TwoBone(const glm::dvec3& shoulder, const glm::dvec3& target, const double lengths[2],
                      const glm::dvec3& pole, const double flex[2], double maxStretch, const glm::dvec3& down) {
    double a = lengths[0], b = lengths[1];
    const glm::dvec3 to = target - shoulder;
    const double d = glm::length(to);
    const glm::dvec3 axis = NormalizeOr(to, down);
    auto reach = [a, b](double f) { return std::sqrt(a * a + b * b + 2.0 * a * b * std::cos(f)); };
    const double dMax = reach(flex[0]), dMin = reach(flex[1]);
    double k = 1.0, dEff = d;
    if (d > dMax) {
        k = std::min(d / dMax, maxStretch);
        dEff = dMax * k;
    } else if (d < dMin) {
        k = std::max(d / dMin, 0.05);
        dEff = std::max(d, dMin * 0.05);
    }
    a *= k;
    b *= k;
    const double cosShoulder = std::clamp((a * a + dEff * dEff - b * b) / (2.0 * a * dEff), -1.0, 1.0);
    const glm::dvec3 side = TryNormalize(RejectFrom(pole, axis)).value_or(AnyOrthonormal(axis));
    const double sinShoulder = std::sqrt(1.0 - cosShoulder * cosShoulder);
    TwoBoneResult r;
    r.elbow = shoulder + (axis * cosShoulder + side * sinShoulder) * a;
    r.wrist = shoulder + axis * dEff;
    r.forearm = NormalizeOr(r.wrist - r.elbow, axis);
    r.fold = NormalizeOr(axis * sinShoulder - side * cosShoulder, -side);
    r.reached = d <= (lengths[0] + lengths[1]) * 1.001;
    return r;
}

namespace {

bool InsideTorso(const ArmSkeleton& skel, const glm::dvec3& local) {
    const double half = TorsoBox[0] * skel.scale, back = TorsoBox[1] * skel.scale;
    const double front = TorsoBox[2] * skel.scale, bottom = TorsoBox[3] * skel.scale;
    return std::abs(local.x) < half && local.z > back && local.z < front && local.y > bottom && local.y < skel.shoulderUp;
}

// The elbow direction this frame, before smoothing
template <typename Solve>
glm::dvec3 SwivelTarget(const ArmSkeleton& skel, const SwivelFrame& frame, double sign, const glm::dvec3& restShoulder,
                        std::optional<glm::dvec3> previous, double straight, const ChestFrame& chest,
                        const HandTarget& target, Solve solve) {
    const glm::dvec3 wrist = target.wrist;
    const glm::dvec3 swivelAxis = glm::cross(frame.u, frame.o);
    // 1. VRArmIK's tuned version of Parger's model, from the hand position in shoulder space
    //    normalized by arm length
    const glm::dvec3 rel = chest.ToLocal(wrist - restShoulder);
    const glm::dvec3 h = rel / skel.ArmLength();
    double alpha = 135.0 - 60.0 * h.y;
    alpha += h.y > 0.0 ? 260.0 * std::max(0.6 - h.z, 0.0) * h.y : -100.0 * std::max(0.6 - h.z, 0.0) * -h.y;
    alpha -= 50.0 * std::max(-h.x * sign + 0.1, 0.0);
    glm::dvec3 prior = frame.Dir(SoftClamp(alpha, 13.0, 175.0, 10.0) * Deg);

    // 2. Parger's singularity correction: near the shoulder's vertical axis and behind the shoulder
    //    the position model is unreliable, so blend towards a fixed direction
    const double radius = glm::length(glm::dvec3{ rel.x, 0.0, rel.z }) / skel.scale;
    const double blend = std::max(1.0 - radius / SingularRadius, std::clamp(-rel.z / skel.scale / SingularBehind, 0.0, 1.0));
    if (blend > 0.0) {
        glm::dvec3 fixed = chest.ToWorld({ PargerFixedElbow.x * sign, PargerFixedElbow.y, PargerFixedElbow.z });
        fixed = NormalizeOr(RejectFrom(fixed, swivelAxis), prior);
        prior = NormalizeOr(glm::mix(prior, fixed, std::clamp(blend, 0.0, 1.0)), prior);
    }

    const glm::dvec3 handY = target.fingers, handZ = target.thumb, handX = glm::cross(handY, handZ);
    double priorAngle = frame.Angle(prior);
    // A nearly straight arm's elbow circle shrinks to a point and the position model says nothing,
    // so there the swivel follows the hand's twist instead: the inside of the elbow towards the
    // thumb, as in a handshake grip
    {
        const glm::dvec3 thumb = RotationArc(handY, solve(frame.Dir(priorAngle)).forearm) * handZ;
        const double untwisted = frame.Angle(RejectFrom(-thumb, swivelAxis));
        priorAngle += WrapAngle(untwisted - priorAngle) * straight;
    }
    std::optional<double> previousAngle;
    if (previous && glm::dot(RejectFrom(*previous, swivelAxis), RejectFrom(*previous, swivelAxis)) > 1e-4)
        previousAngle = frame.Angle(*previous);

    // 3. The cheapest swivel near the prior that keeps the wrist comfortable
    auto excess = [](double angle, double limitDeg) { return std::max(angle - limitDeg * Deg, 0.0); };
    auto cost = [&](double a) {
        const glm::dvec3 e = frame.Dir(a);
        const TwoBoneResult arm = solve(e);
        const glm::dvec3 f = arm.forearm;
        // Wrist bend: where the forearm points, seen from the hand. Towards the thumb side means the
        // hand is bent towards the pinky (ulnar deviation).
        const double along = glm::dot(f, handY);
        const double flex = std::abs(std::atan2(glm::dot(f, handX), along));
        double deviation = std::atan2(glm::dot(f, handZ), along);
        deviation = deviation > 0.0 ? excess(deviation, WristUlnar) : excess(-deviation, WristRadial);
        // Forearm twist: the thumb, swung onto the forearm, against the inside of the elbow
        const glm::dvec3 thumb = RotationArc(handY, f) * handZ;
        const double twist = std::abs(SignedAngle(-e, thumb, f));
        double c = 1.0 - std::cos(a - priorAngle) + WWrist * (std::pow(excess(flex, WristFlex), 2) + deviation * deviation) +
                   WTwist * std::pow(excess(twist, ForearmTwist), 2);
        if (previousAngle)
            c += WPrevious * (1.0 - std::cos(a - *previousAngle));
        if (InsideTorso(skel, chest.ToLocal(arm.elbow - chest.pos)))
            c += WTorso;
        auto outward = [&](const glm::dvec3& v) { return chest.ToLocal(v - chest.pos).x * sign; };
        const double tucked = std::min(outward(restShoulder), outward(wrist)) - outward(arm.elbow);
        c += WInward * std::pow(std::max(tucked, 0.0) / (0.1 * skel.scale), 2);
        return c;
    };
    const double lo = SwivelRangeDeg[0] * Deg, hi = SwivelRangeDeg[1] * Deg, step = SwivelStepDeg * Deg;
    const int count = static_cast<int>(std::round((hi - lo) / step));
    double best = lo, bestCost = std::numeric_limits<double>::max();
    for (int k = 0; k <= count; k++) {
        const double a = lo + step * k;
        const double c = cost(a);
        if (c < bestCost) {
            best = a;
            bestCost = c;
        }
    }
    // Refine with a parabola through the best sample and its neighbours
    const double before = cost(best - step), after = cost(best + step);
    const double curve = before - 2.0 * bestCost + after;
    if (curve > 1e-6) {
        const double refined = std::clamp(best + 0.5 * step * (before - after) / curve, lo, hi);
        if (cost(refined) < bestCost)
            best = refined;
    }
    return frame.Dir(best);
}

// The shoulder girdle's lift scaled down from the skin-marker measurements it comes from: bone pins
// show the clavicle rising less than markers on the skin over it (Ludewig 2009)
constexpr double GirdleRise = 0.7;
// The shoulder girdle follows the arm this quickly (s; the research's 50-80 ms)
constexpr double GirdleTau = 0.06;

// How the shoulder girdle turns about the sternoclavicular joint for an upper arm pointing dir (upper
// chest space, +X outward on either side): its lift (the shoulder end up) and swing forward (back is
// negative), in radians. Klopčar & Lenarčič 2006, Ludewig 2009: see chasm's arm.rs.
glm::dvec2 Girdle(const glm::dvec3& dir) {
    const glm::dvec3 d = NormalizeOr(dir, { 0, -1, 0 });
    const double raised = std::acos(std::clamp(-d.y, -1.0, 1.0)) / Deg;
    const double horizontal = std::sqrt(d.x * d.x + d.z * d.z);
    // The plane it's raised in: 0 out to the side, 90 forward, past that across the body
    const double plane = std::atan2(d.z, d.x) / Deg;
    double lift = raised > 30.0 ? 0.36 * raised - 10.8 : 0.0;
    double forth = raised > 70.0 ? -0.22 * raised + 15.4 : 0.0;
    forth += 7.5 * std::clamp((plane - 35.0) / 55.0, -1.0, 1.0) * Smoothstep(30.0, 120.0, raised);
    if (plane > 35.0)
        forth += std::min(0.25 * (plane - 35.0), 20.0) * horizontal;
    const double behind = std::asin(std::max(-d.z, 0.0)) / Deg;
    lift += 0.3 * behind;
    forth -= 0.35 * behind;
    return { std::clamp(lift * GirdleRise, 0.0, 40.0) * Deg, std::clamp(forth, -25.0, 25.0) * Deg };
}

} // namespace

double EasedReach(double d, double length, double ease) {
    if (ease > 0.0 && d > length - ease) {
        const double u = std::min(d - (length - ease), 2.0 * ease);
        return length - ease + u - u * u / (4.0 * ease);
    }
    return std::min(d, length);
}

glm::dvec3 PlaceShoulder(const ArmParams& p, const ArmSkeleton& skel, double dt, ArmState& state, Side side,
                         const ChestFrame& chest, const glm::dvec3& clavicleRoot, const glm::dvec3& restShoulder, const glm::dvec3& wrist) {
    const double sign = side == Side::Left ? -1.0 : 1.0;
    // Chest space with +X outward on either side
    auto mirror = [sign](const glm::dvec3& v) { return glm::dvec3{ v.x * sign, v.y, v.z }; };
    const double arm = skel.ArmLength();

    // The girdle turns the shoulder about the clavicle's root as the arm last pointed (a frame's lag,
    // under its own smoothing), or as the hand is the first time round
    const glm::dvec3 pointing = state.upper ? *state.upper : mirror(chest.ToLocal(wrist - restShoulder));
    const glm::dvec2 target = Girdle(pointing);
    glm::dvec2 girdle = target;
    if (state.girdle)
        girdle = dt <= 0.0 ? *state.girdle : *state.girdle + (target - *state.girdle) * Smoothing(dt, GirdleTau);
    state.girdle = girdle;
    const glm::dvec3 clavicle = mirror(chest.ToLocal(restShoulder - clavicleRoot));
    const glm::dquat turn = glm::angleAxis(-girdle.y, glm::dvec3{ 0, 1, 0 }) * glm::angleAxis(girdle.x, glm::dvec3{ 0, 0, 1 });
    const glm::dvec3 girdled = clavicleRoot + chest.ToWorld(mirror(turn * clavicle));

    // Near full reach and past it, the shoulder goes the hand's way as a shoulder blade does (forward
    // or across the chest most, up in a shrug, back and down less, out least), taking a growing share
    // of the reaching so the elbow straightens at a steady rate rather than snapping straight
    const glm::dvec3 toHand = wrist - girdled;
    const double reaching = glm::length(toHand);
    const double bendAt = EasedReach(reaching, arm, std::clamp(p.armEase * arm, 0.0, 0.25 * arm));
    double slide = 0.0;
    if (reaching > bendAt) {
        const glm::dvec3 l = mirror(chest.ToLocal(toHand / reaching));
        auto sq = [](double v) { return v * v; };
        const auto& r = p.shoulderReach; // forward, back, up, down, out
        const double reach = skel.scale * (sq(std::max(l.z, 0.0)) * r[0] + sq(std::min(l.x, 0.0)) * r[0] + sq(std::min(l.z, 0.0)) * r[1]
            + sq(std::max(l.y, 0.0)) * r[2] + sq(std::min(l.y, 0.0)) * r[3] + sq(std::max(l.x, 0.0)) * r[4]);
        slide = std::min(reaching - bendAt, reach);
    }
    const glm::dvec3 shoulder = girdled + NormalizeOr(toHand, {}) * slide;
    // Snake's clavicle keeps its length: the shoulder goes as far round its root as that takes it
    return clavicleRoot + NormalizeOr(shoulder - clavicleRoot, girdled - clavicleRoot) * glm::length(clavicle);
}

std::optional<ArmPose> SolveArm(const ArmParams& p, const ArmSkeleton& skel, double dt, ArmState& state, Side side,
                                const ChestFrame& chest, const glm::dvec3& shoulder, const HandTarget& target) {
    const double sign = side == Side::Left ? -1.0 : 1.0;
    const glm::dvec3 up = chest.up, forward = chest.forward, outward = chest.right * sign;
    const glm::dvec3 wrist = target.wrist;
    const double arm = skel.ArmLength();
    const double lengths[2] = { skel.upperArm, skel.lowerArm };

    // FRIK treats a hand this far away as lost tracking
    if (glm::distance(shoulder, wrist) > p.trackingLoss * arm) {
        state.pole.reset();
        return std::nullopt;
    }

    const double flex[2] = { p.elbowMinFlexionDeg * Deg, p.elbowMaxFlexionDeg * Deg };
    const glm::dvec3 axis = NormalizeOr(wrist - shoulder, -up);
    const SwivelFrame frame{ axis, up, forward, outward };
    std::optional<glm::dvec3> previous;
    if (state.pole)
        previous = chest.ToWorld(*state.pole);
    // How straight the arm is: 1 with the elbow all but straight, 0 once it's bent past 30°. Only there
    // does the hand's twist turn the elbow.
    const double a = lengths[0], b = lengths[1];
    const double d = std::min(glm::distance(shoulder, wrist), a + b);
    const double flexion = std::acos(std::clamp((d * d - a * a - b * b) / (2.0 * a * b), -1.0, 1.0));
    const double straight = Smoothstep(30.0 * Deg, 8.0 * Deg, flexion);
    auto solve = [&](const glm::dvec3& e) { return TwoBone(shoulder, wrist, lengths, e, flex, p.armStretch, -up); };
    const glm::dvec3 targetDir = SwivelTarget(skel, frame, sign, shoulder, previous, straight, chest, target, solve);

    // Smooth the swivel, never the hand: the elbow direction is filtered in chest space
    const glm::dvec3 local = chest.ToLocal(targetDir);
    glm::dvec3 poleLocal = local;
    if (state.pole)
        poleLocal = dt <= 0.0 ? *state.pole : NormalizeOr(glm::mix(*state.pole, local, Smoothing(dt, p.swivelTau)), local);
    state.pole = poleLocal;
    glm::dvec3 pole = chest.ToWorld(poleLocal);
    if (glm::dot(RejectFrom(pole, axis), RejectFrom(pole, axis)) <= 1e-6)
        pole = targetDir;

    const TwoBoneResult solved = TwoBone(shoulder, wrist, lengths, pole, flex, p.armStretch, -up);
    ArmPose pose;
    pose.upperDir = NormalizeOr(solved.elbow - shoulder, -up);
    state.upper = chest.ToLocal(pose.upperDir) * glm::dvec3{ sign, 1.0, 1.0 };
    pose.forearmDir = solved.forearm;
    pose.fold = solved.fold;
    // Snake's bones keep their length: the elbow and wrist are placed along the solved directions
    pose.elbow = shoulder + pose.upperDir * a;
    pose.wrist = pose.elbow + pose.forearmDir * b;
    pose.reached = solved.reached;
    return pose;
}

} // namespace ik
