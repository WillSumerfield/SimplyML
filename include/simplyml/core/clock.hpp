#pragma once
#include <chrono>
#include <cstdint>

namespace sml
{

/// Monotonic wall time in seconds since construction, plus per-frame delta. Drives UI animation,
/// so it keeps running while training is paused or the framerate is unlocked.
class Clock
{
public:
    Clock()
        : m_start{std::chrono::steady_clock::now()}
    {}

    [[nodiscard]] double now() const
    {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_start).count();
    }

    /// Marks a new frame; returns seconds since the previous one (0 on the first).
    float tick()
    {
        double const t = now();
        m_dt           = m_frame ? static_cast<float>(t - m_last) : 0.0f;
        m_last         = t;
        ++m_frame;
        return m_dt;
    }

    [[nodiscard]] float         dt() const    { return m_dt; }
    [[nodiscard]] std::uint64_t frame() const { return m_frame; }

private:
    std::chrono::steady_clock::time_point m_start;
    double                                m_last  = 0.0;
    float                                 m_dt    = 0.0f;
    std::uint64_t                         m_frame = 0;
};

/// Simulation time owned by the app: only advances when stepped, honoring pause and time scale.
class SimClock
{
public:
    /// Advances by `dt * scale` unless paused; returns the applied step.
    double step(double dt)
    {
        double const applied = m_paused ? 0.0 : dt * m_scale;
        m_time += applied;
        return applied;
    }

    void reset(double time = 0.0) { m_time = time; }

    [[nodiscard]] double time() const   { return m_time; }
    [[nodiscard]] bool   paused() const { return m_paused; }
    [[nodiscard]] double scale() const  { return m_scale; }

    void setPaused(bool paused)  { m_paused = paused; }
    void togglePause()           { m_paused = !m_paused; }
    void setScale(double scale)  { m_scale = scale; }

private:
    double m_time   = 0.0;
    double m_scale  = 1.0;
    bool   m_paused = false;
};

} // namespace sml
