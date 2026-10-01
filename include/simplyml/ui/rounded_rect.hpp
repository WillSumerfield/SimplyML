#pragma once
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "simplyml/ui/geometry.hpp"

namespace sml
{

/// Rounded rectangle with optional fill, outline (inside the bounds) and drop shadow. Geometry is
/// cached and rebuilt only when a property changes. A transparent fill gives an outline-only card.
class RoundedRect : public sf::Drawable
{
public:
    RoundedRect() = default;
    RoundedRect(sf::FloatRect rect, float radius, sf::Color fill)
        : m_rect{rect}, m_radius{radius}, m_fill{fill}
    {}

    RoundedRect& setRect(sf::FloatRect rect)         { return set(m_rect, rect); }
    RoundedRect& setRadius(float radius)             { return set(m_radius, radius); }
    RoundedRect& setFill(sf::Color color)            { return set(m_fill, color); }
    RoundedRect& setOutline(float thickness, sf::Color color);
    RoundedRect& setShadow(float size, sf::Color color = {0, 0, 0, 50});

    [[nodiscard]] sf::FloatRect rect() const   { return m_rect; }
    [[nodiscard]] float         radius() const { return m_radius; }
    [[nodiscard]] sf::Color     fill() const   { return m_fill; }

    /// Appends this shape's triangles to `va` (Triangles), for batching with other geometry.
    void appendTo(sf::VertexArray& va) const;

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    template<typename T>
    RoundedRect& set(T& field, T const& value)
    {
        if (!(field == value)) {
            field   = value;
            m_dirty = true;
        }
        return *this;
    }

    sf::FloatRect m_rect;
    float         m_radius = 0.0f;
    sf::Color     m_fill   = sf::Color::White;
    float         m_outline = 0.0f;
    sf::Color     m_outlineColor = sf::Color::White;
    float         m_shadow = 0.0f;
    sf::Color     m_shadowColor = {0, 0, 0, 50};

    mutable sf::VertexArray m_va{sf::PrimitiveType::Triangles};
    mutable bool            m_dirty = true;
};

} // namespace sml
