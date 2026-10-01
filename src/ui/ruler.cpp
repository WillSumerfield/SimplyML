#include "simplyml/ui/ruler.hpp"

#include <algorithm>
#include <cmath>

#include <SFML/Graphics/RenderTarget.hpp>

#include "simplyml/ui/geometry.hpp"
#include "simplyml/ui/text.hpp"
#include "simplyml/util/format.hpp"

namespace sml
{

Ruler::Ruler(float from, float to, float step, int majorEvery, sf::Font const& font, Style style)
    : m_font{&font}
    , m_style{std::move(style)}
{
    setRange(from, to, step, majorEvery);
}

void Ruler::setRange(float from, float to, float step, int majorEvery)
{
    m_from  = std::min(from, to);
    m_to    = std::max(from, to);
    m_step  = step > 0.0f ? step : 1.0f;
    m_major = std::max(majorEvery, 1);
}

void Ruler::setStyle(Style style)
{
    m_style = std::move(style);
}

int Ruler::tickCount() const
{
    return static_cast<int>(std::floor((m_to - m_from) / m_step + 1e-4f)) + 1;
}

void Ruler::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    sf::VertexArray va{sf::PrimitiveType::Triangles};
    int const       n = tickCount();
    // Major ticks fall on multiples of step * majorEvery, not on the first tick.
    long const firstIndex = std::lround(m_from / m_step);
    for (int i = 0; i < n; ++i) {
        float const x     = m_from + static_cast<float>(i) * m_step;
        bool const  major = (firstIndex + i) % m_major == 0;
        float const h     = major ? 2.0f * m_style.tickHeight : m_style.tickHeight;
        geo::line(va, {x, 0.0f}, {x, h}, m_style.lineWidth, m_style.color);
    }
    geo::line(va, {m_from, 0.0f}, {m_to, 0.0f}, m_style.lineWidth, m_style.color);
    target.draw(va, states);

    if (!m_font) {
        return;
    }
    for (int i = 0; i < n; ++i) {
        if ((firstIndex + i) % m_major != 0) {
            continue;
        }
        float const       x     = m_from + static_cast<float>(i) * m_step;
        std::string const label = m_style.label ? m_style.label(x) : toString(x, 0);
        sf::RenderStates  s     = states;
        s.transform.translate({x, 2.0f * m_style.tickHeight + m_style.labelGap}).scale({m_style.textScale, m_style.textScale});
        drawText(target, *m_font, label, m_style.textSize, {0.0f, 0.0f}, m_style.color, Align::Center, Align::Start, s);
    }
}

} // namespace sml
