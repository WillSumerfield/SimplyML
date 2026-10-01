#pragma once
#include <algorithm>
#include <cmath>

#include "simplyml/util/math.hpp"

namespace sml
{

/// Easing curve: maps progress t in [0,1] to a ratio that is exactly 0 at t=0 and 1 at t=1.
/// OutBack and OutElastic overshoot 1 in between, by design.
enum class Ease
{
    None,   // jump straight to the end (always 1)
    Linear,
    OutCubic,
    InOutCubic,
    InOutQuint,
    InOutExpo,
    InOutCirc,
    OutBack,
    OutElastic,
    Sigmoid,
};

namespace ease
{

inline float linear(float t)
{
    return t;
}

inline float outCubic(float t)
{
    float const u = 1.0f - t;
    return 1.0f - u * u * u;
}

inline float inOutCubic(float t)
{
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    }
    float const u = -2.0f * t + 2.0f;
    return 1.0f - u * u * u * 0.5f;
}

inline float inOutQuint(float t)
{
    if (t < 0.5f) {
        float const t2 = t * t;
        return 16.0f * t2 * t2 * t;
    }
    float const u  = -2.0f * t + 2.0f;
    float const u2 = u * u;
    return 1.0f - u2 * u2 * u * 0.5f;
}

inline float inOutExpo(float t)
{
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }
    if (t < 0.5f) {
        return std::exp2(20.0f * t - 10.0f) * 0.5f;
    }
    return (2.0f - std::exp2(-20.0f * t + 10.0f)) * 0.5f;
}

inline float inOutCirc(float t)
{
    if (t < 0.5f) {
        float const u = 2.0f * t;
        return (1.0f - std::sqrt(std::max(0.0f, 1.0f - u * u))) * 0.5f;
    }
    float const u = -2.0f * t + 2.0f;
    return (std::sqrt(std::max(0.0f, 1.0f - u * u)) + 1.0f) * 0.5f;
}

inline float outBack(float t)
{
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    float const u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

inline float outElastic(float t)
{
    constexpr float c4 = TwoPi / 3.0f;
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }
    return std::exp2(-10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

/// Logistic curve rescaled so it hits 0 and 1 exactly at the ends.
inline float sigmoid(float t)
{
    constexpr float k = 20.0f;
    float const lo = 1.0f / (1.0f + std::exp(k * 0.5f));
    float const hi = 1.0f / (1.0f + std::exp(-k * 0.5f));
    float const s  = 1.0f / (1.0f + std::exp(-k * (t - 0.5f)));
    return (s - lo) / (hi - lo);
}

} // namespace ease

/// Eased ratio for progress `t`; `t` is clamped to [0,1] first.
inline float applyEase(Ease e, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    switch (e) {
        case Ease::None:       return 1.0f;
        case Ease::Linear:     return ease::linear(t);
        case Ease::OutCubic:   return ease::outCubic(t);
        case Ease::InOutCubic: return ease::inOutCubic(t);
        case Ease::InOutQuint: return ease::inOutQuint(t);
        case Ease::InOutExpo:  return ease::inOutExpo(t);
        case Ease::InOutCirc:  return ease::inOutCirc(t);
        case Ease::OutBack:    return ease::outBack(t);
        case Ease::OutElastic: return ease::outElastic(t);
        case Ease::Sigmoid:    return ease::sigmoid(t);
    }
    return t;
}

} // namespace sml
