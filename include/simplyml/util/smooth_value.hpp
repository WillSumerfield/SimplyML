#pragma once
#include <algorithm>

#include "simplyml/util/easing.hpp"

namespace sml
{

/// Value that eases toward a target over a fixed duration. Time (seconds) is supplied by the caller,
/// so it works with any clock. T needs `T + T`, `T - T`, `T * float` and `==`.
template<typename T>
class SmoothValue
{
public:
    SmoothValue() = default;

    explicit SmoothValue(T const& value, float duration = 0.25f, Ease ease = Ease::InOutExpo)
        : m_start{value}
        , m_target{value}
        , m_duration{duration}
        , m_ease{ease}
    {}

    /// Starts easing from the current value toward `target`. No-op if `target` is already the target,
    /// so it is safe to call every frame.
    void set(T const& target, double now)
    {
        if (target == m_target) {
            return;
        }
        m_start     = get(now);
        m_target    = target;
        m_startTime = now;
        m_active    = m_duration;
    }

    void setInstant(T const& value)
    {
        m_start  = value;
        m_target = value;
    }

    [[nodiscard]] T get(double now) const
    {
        float const p = progress(now);
        if (p >= 1.0f) {
            return m_target;
        }
        return m_start + (m_target - m_start) * applyEase(m_ease, p);
    }

    /// Raw (un-eased) progress of the current animation in [0,1].
    [[nodiscard]] float progress(double now) const
    {
        if (m_active <= 0.0f) {
            return 1.0f;
        }
        return std::clamp(static_cast<float>((now - m_startTime) / m_active), 0.0f, 1.0f);
    }

    [[nodiscard]] bool     done(double now) const { return progress(now) >= 1.0f; }
    [[nodiscard]] T const& target() const         { return m_target; }
    [[nodiscard]] float    duration() const       { return m_duration; }
    [[nodiscard]] Ease     ease() const           { return m_ease; }

    /// Takes effect from the next `set()`.
    void setDuration(float duration) { m_duration = duration; }
    void setEase(Ease ease)          { m_ease = ease; }

private:
    T      m_start{};
    T      m_target{};
    double m_startTime = 0.0;
    float  m_duration  = 0.25f;
    float  m_active    = 0.0f; // duration of the running animation
    Ease   m_ease      = Ease::InOutExpo;
};

using SmoothFloat = SmoothValue<float>;

} // namespace sml
