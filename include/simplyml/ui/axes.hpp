#pragma once
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "simplyml/ui/value_format.hpp"
#include "simplyml/ui/widget.hpp"
#include "simplyml/util/smooth_value.hpp"

namespace sml
{

/// A 1-2-5 x 10^k spacing giving about `target` intervals over `range`.
[[nodiscard]] double niceStep(double range, int target);

/// Ticks covering [lo, hi]: `range` widened to multiples of `step`, plus every tick inside.
struct Ticks
{
    double              lo   = 0.0;
    double              hi   = 1.0;
    double              step = 1.0;
    std::vector<double> values;
};
/// Flat or empty input is padded to a usable range first. `integer` keeps steps >= 1.
[[nodiscard]] Ticks niceTicks(double lo, double hi, int target, bool integer = false);

/// Data -> pixel mapping for one plot rectangle (y grows up).
struct AxisMap
{
    sf::FloatRect rect;
    double        x0 = 0.0, x1 = 1.0;
    double        y0 = 0.0, y1 = 1.0;

    [[nodiscard]] float x(double v) const
    {
        return rect.position.x + static_cast<float>((v - x0) / (x1 - x0)) * rect.size.x;
    }
    [[nodiscard]] float y(double v) const
    {
        return rect.position.y + rect.size.y - static_cast<float>((v - y0) / (y1 - y0)) * rect.size.y;
    }
};

/// Y range that eases toward new targets so charts rescale smoothly.
class SmoothRange
{
public:
    void set(double lo, double hi, double now);
    [[nodiscard]] double lo(double now) const { return m_lo.get(now); }
    [[nodiscard]] double hi(double now) const { return m_hi.get(now); }
    [[nodiscard]] bool   moving(double now) const { return !m_lo.done(now) || !m_hi.done(now); }

private:
    bool                m_init = false;
    SmoothValue<double> m_lo{0.0, 0.35f, Ease::OutCubic};
    SmoothValue<double> m_hi{1.0, 0.35f, Ease::OutCubic};
};

/// Grid lines and tick labels shared by charts. Labels for y sit at the right edge just above
/// their line; labels for x sit centered below the plot rectangle.
void addGridLines(sf::VertexArray& tris, AxisMap const& map, std::vector<double> const& xs,
                  std::vector<double> const& ys, UiContext const& ctx);
void drawYLabels(sf::RenderTarget& target, AxisMap const& map, std::vector<double> const& ys,
                 ValueFormat const& fmt, UiContext const& ctx);
void drawXLabels(sf::RenderTarget& target, AxisMap const& map, std::vector<double> const& xs,
                 ValueFormat const& fmt, UiContext const& ctx);

} // namespace sml
