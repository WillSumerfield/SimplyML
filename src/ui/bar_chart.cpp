#include "simplyml/ui/bar_chart.hpp"

#include <algorithm>
#include <cmath>

#include "simplyml/ui/geometry.hpp"
#include "simplyml/ui/text.hpp"

namespace sml
{

BarChart::BarChart(std::string title, std::string series, sf::Color color)
    : Panel{std::move(title), color}
    , m_series{std::move(series)}
{}

BarChart& BarChart::setWindow(std::size_t bars)
{
    m_window = std::max<std::size_t>(bars, 1);
    m_dirty  = true;
    return *this;
}

BarChart& BarChart::setYRange(double lo, double hi)
{
    m_autoY   = false;
    m_fixedLo = lo;
    m_fixedHi = hi;
    m_seen    = UINT64_MAX;
    return *this;
}

BarChart& BarChart::setAutoY()
{
    m_autoY = true;
    m_seen  = UINT64_MAX;
    return *this;
}

void BarChart::onLayout(UiContext const& ctx)
{
    Panel::onLayout(ctx);
    m_seen = UINT64_MAX; // ticks depend on the height
}

void BarChart::update(UiContext const& ctx)
{
    Panel::update(ctx);
    if (!accent().a) {
        setAccent(ctx.theme.palette.blue);
    }
    Series const*       s     = ctx.store.series(m_series);
    std::uint64_t const total = s ? s->total() : 0;
    if (total != m_seen) {
        m_seen = total;
        double lo = 0.0, hi = 0.0; // bars grow from zero
        if (s) {
            std::size_t const n = s->size();
            for (std::size_t i = n > m_window ? n - m_window : 0; i < n; ++i) {
                double const v = s->points()[i].value;
                if (std::isfinite(v)) {
                    lo = std::min(lo, v);
                    hi = std::max(hi, v);
                }
            }
        }
        if (!m_autoY) {
            lo = m_fixedLo;
            hi = m_fixedHi;
        }
        int const target = std::clamp(static_cast<int>(contentRect().size.y / ctx.theme.px(70.0f)), 2, 8);
        m_yTicks = niceTicks(lo, hi, target);
        if (!m_autoY) {
            m_yTicks.lo = m_fixedLo;
            m_yTicks.hi = m_fixedHi;
        }
        m_range.set(m_yTicks.lo, m_yTicks.hi, ctx.now);
        m_dirty = true;
        setValueText(s && !s->empty() ? m_valueFormat(s->last().value) : std::string{});
    }
    if (m_range.moving(ctx.now)) {
        m_dirty = true;
    }
}

void BarChart::rebuild(UiContext const& ctx)
{
    m_dirty = false;
    m_grid.clear();
    m_mesh.clear();
    m_xLabels.clear();
    m_xLabelPos.clear();
    m_map.rect = contentRect();
    m_map.x0   = 0.0;
    m_map.x1   = 1.0;
    m_map.y0   = m_range.lo(ctx.now);
    m_map.y1   = std::max(m_range.hi(ctx.now), m_map.y0 + 1e-12);
    addGridLines(m_grid, m_map, {}, m_yTicks.values, ctx);

    Series const* s = ctx.store.series(m_series);
    if (!s || s->empty() || m_map.rect.size.x <= 0.0f) {
        return;
    }
    Theme const&      t     = ctx.theme;
    sf::Color const   color = accent();
    std::size_t const n     = s->size();
    std::size_t const first = n > m_window ? n - m_window : 0;
    // Bar slots are sized for the full window, so bars keep their width as data arrives (unless filling).
    float const slot  = m_map.rect.size.x / static_cast<float>(m_fill ? n - first : m_window);
    float const space = std::min(t.px(2.0f), 0.25f * slot);
    float const zeroY = std::clamp(m_map.y(0.0), m_map.rect.position.y, m_map.rect.position.y + m_map.rect.size.y);
    float const cap   = t.px(4.0f);
    // Label every k-th bar so labels don't collide.
    float const       labelWidth = textSize(ctx.font, m_xFormat(s->last().step), t.pt(t.textTick)).x + t.px(16.0f);
    std::size_t const every      = std::max<std::size_t>(1, static_cast<std::size_t>(std::ceil(labelWidth / slot)));

    for (std::size_t i = first; i < n; ++i) {
        auto const& p = s->points()[i];
        float const x = m_map.rect.position.x + static_cast<float>(i - first) * slot;
        float const w = slot - space;
        float const y = std::clamp(m_map.y(p.value), m_map.rect.position.y, m_map.rect.position.y + m_map.rect.size.y);
        float const top    = std::min(y, zeroY);
        float const height = std::abs(zeroY - y);
        geo::rect(m_mesh, {{x, top}, {w, height}}, {color.r, color.g, color.b, 50});
        float const c = std::min(cap, height);
        geo::rect(m_mesh, {{x, p.value >= 0.0 ? y : y - c}, {w, c}}, color);
        if ((i - first) % every == 0) {
            m_xLabels.push_back(p.step);
            m_xLabelPos.push_back(x + 0.5f * w);
        }
    }
}

void BarChart::drawContent(sf::RenderTarget& target, UiContext const& ctx)
{
    if (m_dirty) {
        rebuild(ctx);
    }
    target.draw(m_grid);
    target.draw(m_mesh);
    ValueFormat const yfmt = m_yFormatSet ? m_yFormat : ValueFormat::number(decimalsFor(m_yTicks.step));
    drawYLabels(target, m_map, m_yTicks.values, yfmt, ctx);
    Theme const& t = ctx.theme;
    for (std::size_t i = 0; i < m_xLabels.size(); ++i) {
        drawText(target, ctx.font, m_xFormat(m_xLabels[i]), t.pt(t.textTick),
                 {m_xLabelPos[i], m_map.rect.position.y + m_map.rect.size.y + t.px(8.0f)}, t.palette.textDim,
                 Align::Center, Align::Start);
    }
}

} // namespace sml
