#include "simplyml/ui/phase_plot.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "simplyml/ui/geometry.hpp"

namespace sml
{

PhasePlot::PhasePlot(std::string title, std::string xSeries, std::string ySeries, sf::Color color)
    : Panel{std::move(title), color}
    , m_x{std::move(xSeries)}
    , m_y{std::move(ySeries)}
{}

PhasePlot& PhasePlot::setSeries(std::string x, std::string y)
{
    m_x     = std::move(x);
    m_y     = std::move(y);
    m_seenX = m_seenY = UINT64_MAX;
    return *this;
}

PhasePlot& PhasePlot::setMaxPoints(std::size_t points)
{
    m_maxPoints = std::max<std::size_t>(points, 2);
    m_dirty     = true;
    return *this;
}

PhasePlot& PhasePlot::setRanges(double xlo, double xhi, double ylo, double yhi)
{
    m_auto = false;
    m_xlo = xlo, m_xhi = xhi, m_ylo = ylo, m_yhi = yhi;
    m_dirty = true;
    return *this;
}

PhasePlot& PhasePlot::setAutoRanges()
{
    m_auto  = true;
    m_dirty = true;
    return *this;
}

void PhasePlot::onLayout(UiContext const& ctx)
{
    Panel::onLayout(ctx);
    m_dirty = true;
}

void PhasePlot::update(UiContext const& ctx)
{
    Panel::update(ctx);
    if (!accent().a) {
        setAccent(ctx.theme.palette.accent);
    }
    Series const* xs = ctx.store.series(m_x);
    Series const* ys = ctx.store.series(m_y);
    std::uint64_t const tx = xs ? xs->total() : 0;
    std::uint64_t const ty = ys ? ys->total() : 0;
    if (tx != m_seenX || ty != m_seenY) {
        m_seenX = tx;
        m_seenY = ty;
        m_dirty = true;
    }
}

void PhasePlot::rebuild(UiContext const& ctx)
{
    m_dirty = false;
    m_mesh.clear();
    Series const* xs = ctx.store.series(m_x);
    Series const* ys = ctx.store.series(m_y);
    if (!xs || !ys) {
        return;
    }
    // Pair by index from the newest backwards (series may have different lengths).
    std::size_t const n = std::min({xs->size(), ys->size(), m_maxPoints});
    if (n < 2) {
        return;
    }
    std::size_t const bx = xs->size() - n;
    std::size_t const by = ys->size() - n;

    double xlo = m_xlo, xhi = m_xhi, ylo = m_ylo, yhi = m_yhi;
    if (m_auto) {
        xlo = ylo = std::numeric_limits<double>::infinity();
        xhi = yhi = -xlo;
        for (std::size_t i = 0; i < n; ++i) {
            double const x = xs->points()[bx + i].value;
            double const y = ys->points()[by + i].value;
            xlo = std::min(xlo, x), xhi = std::max(xhi, x);
            ylo = std::min(ylo, y), yhi = std::max(yhi, y);
        }
        auto pad = [](double& lo, double& hi) {
            double const r = std::max(hi - lo, 1e-6);
            lo -= 0.05 * r;
            hi += 0.05 * r;
        };
        pad(xlo, xhi);
        pad(ylo, yhi);
    }
    m_map = {contentRect(), xlo, xhi, ylo, yhi};

    std::vector<sf::Vector2f> pts(n);
    std::vector<float>        widths(n);
    float const               scale = ctx.theme.scale;
    for (std::size_t i = 0; i < n; ++i) {
        pts[i] = {m_map.x(xs->points()[bx + i].value), m_map.y(ys->points()[by + i].value)};
        // Newest ~30 points taper from the newest width to the oldest.
        float const age = static_cast<float>(n - 1 - i);
        widths[i] = scale * (m_oldest + (m_newest - m_oldest) * std::exp(-age / 30.0f));
    }
    geo::polyline(m_mesh, pts.data(), n, 0.0f, accent(), widths.data());
    geo::circle(m_mesh, pts.back(), scale * m_newest, accent(), 16);
}

void PhasePlot::drawContent(sf::RenderTarget& target, UiContext const& ctx)
{
    if (m_dirty) {
        rebuild(ctx);
    }
    target.draw(m_mesh);
}

} // namespace sml
