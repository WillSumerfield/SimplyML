#pragma once
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "simplyml/util/ring_buffer.hpp"

namespace sml
{

/// Fading trail of a moving point (e.g. a pendulum tip), drawn in its own units (usually world
/// space through the Camera). Keeps the newest `capacity` points; each point's width eases from
/// `widthNew` to `widthOld` over `fadeTime` seconds after it was added.
class Tracer : public sf::Drawable
{
public:
    explicit Tracer(std::size_t capacity = 200, sf::Color color = sf::Color::White);

    void add(sf::Vector2f point, double now);
    void clear() { m_points.clear(); }
    /// Rebuilds the mesh for time `now`; call once per frame before drawing.
    void update(double now);

    void setColor(sf::Color color) { m_color = color; }
    void setWidths(float widthNew, float widthOld) { m_widthNew = widthNew; m_widthOld = widthOld; }
    void setFadeTime(float seconds) { m_fade = seconds; }
    void setCapacity(std::size_t capacity) { m_points.setCapacity(capacity); }

    [[nodiscard]] std::size_t size() const { return m_points.size(); }

private:
    struct Point
    {
        sf::Vector2f pos;
        double       time = 0.0;
        Point operator+(Point const& o) const { return {pos + o.pos, time + o.time}; }
        Point operator-(Point const& o) const { return {pos - o.pos, time - o.time}; }
        Point operator/(float d) const { return {pos / d, time / d}; }
    };

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    RingBuffer<Point> m_points;
    sf::Color         m_color;
    float             m_widthNew = 4.0f;
    float             m_widthOld = 0.0f;
    float             m_fade     = 1.0f;
    sf::VertexArray   m_mesh{sf::PrimitiveType::Triangles};
};

} // namespace sml
