#include "simplyml/ui/tracer.hpp"

#include <algorithm>
#include <vector>

#include <SFML/Graphics/RenderTarget.hpp>

#include "simplyml/ui/geometry.hpp"
#include "simplyml/util/easing.hpp"

namespace sml
{

Tracer::Tracer(std::size_t capacity, sf::Color color)
    : m_points{capacity}
    , m_color{color}
{}

void Tracer::add(sf::Vector2f point, double now)
{
    m_points.push({point, now});
}

void Tracer::update(double now)
{
    m_mesh.clear();
    std::size_t const n = m_points.size();
    if (n < 2) {
        return;
    }
    std::vector<sf::Vector2f> pts(n);
    std::vector<float>        widths(n);
    for (std::size_t i = 0; i < n; ++i) {
        Point const& p = m_points[i];
        float const  t = m_fade > 0.0f ? static_cast<float>((now - p.time) / m_fade) : 1.0f;
        pts[i]    = p.pos;
        widths[i] = m_widthNew + (m_widthOld - m_widthNew) * applyEase(Ease::InOutQuint, t);
    }
    geo::polyline(m_mesh, pts.data(), n, 0.0f, m_color, widths.data());
}

void Tracer::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_mesh, states);
}

} // namespace sml
