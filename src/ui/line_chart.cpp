#include "simplyml/ui/line_chart.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "simplyml/ui/geometry.hpp"

namespace sml
{

namespace
{

/// Visible slice [begin, end) of a series.
std::pair<std::size_t, std::size_t> visibleSlice(Series const& s, std::size_t window)
{
    std::size_t const n = s.size();
    return {window && n > window ? n - window : 0, n};
}

} // namespace

LineChart::LineChart(std::string title, std::string series, sf::Color color)
    : Panel{std::move(title), color}
{
    if (!series.empty()) {
        addSeries(std::move(series));
    }
}

LineChart& LineChart::addSeries(std::string name, sf::Color color, std::string label)
{
    if (label.empty()) {
        label = name;
    }
    m_lines.push_back({std::move(name), std::move(label), color});
    m_dirty = true;
    return *this;
}

LineChart& LineChart::setYRange(double lo, double hi)
{
    m_autoY   = false;
    m_fixedLo = lo;
    m_fixedHi = hi;
    m_dirty   = true;
    return *this;
}

LineChart& LineChart::setAutoY(bool includeZero)
{
    m_autoY       = true;
    m_includeZero = includeZero;
    m_dirty       = true;
    return *this;
}

sf::Color LineChart::colorOf(std::size_t i, Theme const& theme) const
{
    sf::Color const c = m_lines[i].color;
    if (c.a) {
        return c;
    }
    if (i == 0 && accent().a) {
        return accent();
    }
    return theme.palette.series(i);
}

void LineChart::onLayout(UiContext const& ctx)
{
    Panel::onLayout(ctx);
    m_dirty = true;
}

void LineChart::update(UiContext const& ctx)
{
    Panel::update(ctx);
    if (!accent().a && !m_lines.empty()) {
        setAccent(colorOf(0, ctx.theme));
    }

    // Data extents over the visible windows; detect new points.
    double xlo = std::numeric_limits<double>::infinity(), xhi = -xlo;
    double ylo = xlo, yhi = -xlo;
    bool   changed = false;
    m_hasData = false;
    for (auto& l : m_lines) {
        Series const* s = ctx.store.series(l.name);
        std::uint64_t const total = s ? s->total() : 0;
        std::size_t const   size  = s ? s->size() : 0;
        if (total != l.seenTotal || size != l.seenSize) {
            l.seenTotal = total;
            l.seenSize  = size;
            changed     = true;
        }
        if (!s || s->empty()) {
            continue;
        }
        m_hasData = true;
        auto const [b, e] = visibleSlice(*s, m_window);
        auto const& pts   = s->points();
        xlo = std::min(xlo, pts[b].step);
        xhi = std::max(xhi, pts[e - 1].step);
        for (std::size_t i = b; i < e; ++i) {
            if (std::isfinite(pts[i].value)) {
                ylo = std::min(ylo, pts[i].value);
                yhi = std::max(yhi, pts[i].value);
            }
        }
    }

    if (changed || m_dirty) {
        if (!m_hasData || !(ylo <= yhi)) {
            ylo = 0.0;
            yhi = 1.0;
        }
        if (m_autoY && m_includeZero) {
            ylo = std::min(ylo, 0.0);
            yhi = std::max(yhi, 0.0);
        }
        if (!m_autoY) {
            ylo = m_fixedLo;
            yhi = m_fixedHi;
        }
        int const yTarget = std::clamp(static_cast<int>(contentRect().size.y / ctx.theme.px(70.0f)), 2, 8);
        m_yTicks = niceTicks(ylo, yhi, yTarget);
        if (!m_autoY) { // keep the requested range exactly
            m_yTicks.lo = m_fixedLo;
            m_yTicks.hi = m_fixedHi;
        }
        m_range.set(m_yTicks.lo, m_yTicks.hi, ctx.now);

        if (!(xlo <= xhi)) {
            xlo = 0.0;
            xhi = 1.0;
        }
        if (xhi - xlo < 1e-12) {
            xlo -= 1.0;
            xhi += 1.0;
        }
        m_map.x0 = xlo;
        m_map.x1 = xhi;
        int const xTarget = std::clamp(static_cast<int>(contentRect().size.x / ctx.theme.px(130.0f)), 1, 12);
        Ticks const xt    = niceTicks(xlo, xhi, xTarget, true);
        m_xTicks.clear();
        for (double v : xt.values) {
            if (v >= xlo - 1e-9 && v <= xhi + 1e-9) {
                m_xTicks.push_back(v);
            }
        }
        m_dirty = true;
    }
    if (m_range.moving(ctx.now)) {
        m_dirty = true;
    }

    // Readout or legend.
    if (m_lines.size() == 1) {
        Series const* s = ctx.store.series(m_lines[0].name);
        setValueText(s && !s->empty() ? m_valueFormat(s->last().value) : std::string{});
    } else {
        std::vector<LegendEntry> legend;
        for (std::size_t i = 0; i < m_lines.size(); ++i) {
            legend.push_back({m_lines[i].label, colorOf(i, ctx.theme)});
        }
        setLegend(std::move(legend));
    }
}

void LineChart::rebuild(UiContext const& ctx)
{
    m_dirty = false;
    m_grid.clear();
    m_mesh.clear();
    m_map.rect = contentRect();
    m_map.y0   = m_range.lo(ctx.now);
    m_map.y1   = m_range.hi(ctx.now);
    if (m_map.y1 - m_map.y0 < 1e-12) {
        m_map.y1 = m_map.y0 + 1.0;
    }
    addGridLines(m_grid, m_map, m_xTicks, m_yTicks.values, ctx);
    if (!m_hasData || m_map.rect.size.x <= 0.0f || m_map.rect.size.y <= 0.0f) {
        return;
    }

    float const top    = m_map.rect.position.y;
    float const bottom = top + m_map.rect.size.y;
    float const baseY  = std::clamp(m_map.y(0.0), top, bottom);
    float const width  = 2.0f * ctx.theme.px(ctx.theme.lineWidth);
    bool const  single = m_lines.size() == 1;

    std::vector<sf::Vector2f> pts;
    std::vector<sf::Color>    tops;
    for (std::size_t li = 0; li < m_lines.size(); ++li) {
        Series const* s = ctx.store.series(m_lines[li].name);
        if (!s || s->empty()) {
            continue;
        }
        sf::Color const color = colorOf(li, ctx.theme);
        auto const [b, e]     = visibleSlice(*s, m_window);
        auto const& data      = s->points();

        // Min/max per pixel column once there are more points than pixels.
        pts.clear();
        std::size_t const n       = e - b;
        std::size_t const columns = static_cast<std::size_t>(std::max(1.0f, m_map.rect.size.x));
        double maxAbs = 0.0;
        if (n > 2 * columns) {
            std::size_t i = b;
            for (std::size_t c = 0; c < columns && i < e; ++c) {
                std::size_t const end = b + (n * (c + 1)) / columns;
                std::size_t lo = i, hi = i;
                for (std::size_t j = i; j < end; ++j) {
                    if (data[j].value < data[lo].value) lo = j;
                    if (data[j].value > data[hi].value) hi = j;
                }
                for (std::size_t j : {std::min(lo, hi), std::max(lo, hi)}) {
                    pts.push_back({m_map.x(data[j].step), m_map.y(data[j].value)});
                    maxAbs = std::max(maxAbs, std::abs(data[j].value));
                }
                i = end;
            }
        } else {
            for (std::size_t j = b; j < e; ++j) {
                pts.push_back({m_map.x(data[j].step), m_map.y(data[j].value)});
                maxAbs = std::max(maxAbs, std::abs(data[j].value));
            }
        }
        for (auto& p : pts) {
            p.y = std::clamp(p.y, top, bottom);
        }

        if (pts.size() == 1) {
            geo::circle(m_mesh, pts[0], width, color, 16);
            continue;
        }
        if (m_area) {
            // Fill fades with |value| relative to the largest visible magnitude, as in the original.
            tops.resize(pts.size());
            float const maxAlpha = single ? 150.0f : 60.0f;
            for (std::size_t k = 0; k < pts.size(); ++k) {
                double const v     = m_map.y0 + (bottom - pts[k].y) / m_map.rect.size.y * (m_map.y1 - m_map.y0);
                float const  ratio = maxAbs > 0.0 ? static_cast<float>(std::abs(v) / maxAbs) : 0.0f;
                tops[k] = {color.r, color.g, color.b,
                           static_cast<std::uint8_t>(std::min(1.0f, 2.0f * ratio) * maxAlpha)};
            }
            geo::area(m_mesh, pts.data(), pts.size(), baseY, tops.data(), {color.r, color.g, color.b, 0});
        }
        geo::polyline(m_mesh, pts.data(), pts.size(), width, color);
    }
}

void LineChart::drawContent(sf::RenderTarget& target, UiContext const& ctx)
{
    if (m_dirty) {
        rebuild(ctx);
    }
    target.draw(m_grid);
    target.draw(m_mesh);
    ValueFormat const yfmt = m_yFormatSet ? m_yFormat : ValueFormat::number(decimalsFor(m_yTicks.step));
    drawYLabels(target, m_map, m_yTicks.values, yfmt, ctx);
    drawXLabels(target, m_map, m_xTicks, m_xFormat, ctx);
}

} // namespace sml
