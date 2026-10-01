#include "simplyml/ui/axes.hpp"

#include <algorithm>
#include <cmath>

#include "simplyml/ui/geometry.hpp"
#include "simplyml/ui/text.hpp"

namespace sml
{

double niceStep(double range, int target)
{
    if (!(range > 0.0) || !std::isfinite(range)) {
        return 1.0;
    }
    double const raw  = range / std::max(target, 1);
    double const mag  = std::pow(10.0, std::floor(std::log10(raw)));
    double const norm = raw / mag;
    double const nice = norm < 1.5 ? 1.0 : norm < 3.0 ? 2.0 : norm < 7.0 ? 5.0 : 10.0;
    return nice * mag;
}

Ticks niceTicks(double lo, double hi, int target, bool integer)
{
    if (!std::isfinite(lo) || !std::isfinite(hi)) {
        lo = 0.0;
        hi = 1.0;
    }
    if (hi < lo) {
        std::swap(lo, hi);
    }
    if (hi - lo <= 1e-12 * std::max(1.0, std::abs(hi))) { // flat data
        double const pad = std::abs(lo) > 0.0 ? std::abs(lo) * 0.1 : 1.0;
        lo -= pad;
        hi += pad;
    }
    Ticks t;
    t.step = niceStep(hi - lo, target);
    if (integer) {
        t.step = std::max(t.step, 1.0);
    }
    t.lo = std::floor(lo / t.step + 1e-9) * t.step;
    t.hi = std::ceil(hi / t.step - 1e-9) * t.step;
    for (double v = t.lo; v <= t.hi + 0.5 * t.step; v += t.step) {
        t.values.push_back(std::abs(v) < 1e-12 * t.step ? 0.0 : v);
    }
    return t;
}

void SmoothRange::set(double lo, double hi, double now)
{
    if (!m_init) {
        m_lo.setInstant(lo);
        m_hi.setInstant(hi);
        m_init = true;
        return;
    }
    m_lo.set(lo, now);
    m_hi.set(hi, now);
}

void addGridLines(sf::VertexArray& tris, AxisMap const& map, std::vector<double> const& xs,
                  std::vector<double> const& ys, UiContext const& ctx)
{
    Theme const&    t     = ctx.theme;
    sf::Color const color = {t.palette.grey.r, t.palette.grey.g, t.palette.grey.b, 70};
    float const     w     = std::max(1.0f, t.px(t.tickLine));
    sf::FloatRect const r = map.rect;
    for (double v : ys) {
        float const y = std::round(map.y(v)) + 0.5f;
        if (y >= r.position.y - 0.5f && y <= r.position.y + r.size.y + 0.5f) {
            geo::line(tris, {r.position.x, y}, {r.position.x + r.size.x, y}, w, color);
        }
    }
    for (double v : xs) {
        float const x = std::round(map.x(v)) + 0.5f;
        if (x >= r.position.x - 0.5f && x <= r.position.x + r.size.x + 0.5f) {
            geo::line(tris, {x, r.position.y}, {x, r.position.y + r.size.y}, w, color);
        }
    }
}

void drawYLabels(sf::RenderTarget& target, AxisMap const& map, std::vector<double> const& ys,
                 ValueFormat const& fmt, UiContext const& ctx)
{
    Theme const& t   = ctx.theme;
    unsigned const size = t.pt(t.textTick);
    float const    cap  = capHeight(ctx.font, size);
    sf::FloatRect const r = map.rect;
    for (double v : ys) {
        float const y = map.y(v);
        if (y - cap < r.position.y - 1.0f || y > r.position.y + r.size.y + 1.0f) {
            continue;
        }
        drawText(target, ctx.font, fmt(v), size, {r.position.x + r.size.x - t.px(4.0f), y - t.px(5.0f)},
                 t.palette.textDim, Align::End, Align::End);
    }
}

void drawXLabels(sf::RenderTarget& target, AxisMap const& map, std::vector<double> const& xs,
                 ValueFormat const& fmt, UiContext const& ctx)
{
    Theme const& t = ctx.theme;
    sf::FloatRect const r = map.rect;
    for (double v : xs) {
        float const x = map.x(v);
        if (x < r.position.x - 1.0f || x > r.position.x + r.size.x + 1.0f) {
            continue;
        }
        drawText(target, ctx.font, fmt(v), t.pt(t.textTick), {x, r.position.y + r.size.y + t.px(8.0f)},
                 t.palette.textDim, Align::Center, Align::Start);
    }
}

} // namespace sml
