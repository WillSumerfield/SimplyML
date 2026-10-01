#include "simplyml/ui/rounded_rect.hpp"

#include <SFML/Graphics/RenderTarget.hpp>

namespace sml
{

RoundedRect& RoundedRect::setOutline(float thickness, sf::Color color)
{
    set(m_outline, thickness);
    return set(m_outlineColor, color);
}

RoundedRect& RoundedRect::setShadow(float size, sf::Color color)
{
    set(m_shadow, size);
    return set(m_shadowColor, color);
}

void RoundedRect::appendTo(sf::VertexArray& va) const
{
    geo::roundedShadow(va, m_rect, m_radius, m_shadow, m_shadowColor);
    if (m_fill.a) {
        // Under an opaque outline the fill spans the whole rect (no hairline seams); under a
        // translucent one it stops at the outline so the two don't blend.
        sf::FloatRect inner = m_rect;
        float         r     = m_radius;
        if (m_outline > 0.0f && m_outlineColor.a < 255) {
            inner = {m_rect.position + sf::Vector2f{m_outline, m_outline},
                     m_rect.size - sf::Vector2f{2 * m_outline, 2 * m_outline}};
            r     = m_radius - m_outline;
        }
        geo::roundedRect(va, inner, r, m_fill);
    }
    if (m_outline > 0.0f && m_outlineColor.a) {
        geo::roundedRing(va, m_rect, m_radius, m_outline, m_outlineColor);
    }
}

void RoundedRect::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (m_dirty) {
        m_va.clear();
        appendTo(m_va);
        m_dirty = false;
    }
    target.draw(m_va, states);
}

} // namespace sml
