#pragma once
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "simplyml/core/camera.hpp"

namespace sml
{

/// A render target plus the camera, with separate world-space and screen-space draw paths.
class Canvas
{
public:
    Canvas(sf::RenderTarget& target, Camera const& camera)
        : m_target{target}
        , m_camera{camera}
    {}

    /// Draws through the camera; `states.transform` is applied in world space.
    void drawWorld(sf::Drawable const& d, sf::RenderStates states = sf::RenderStates::Default)
    {
        states.transform = m_camera.transform() * states.transform;
        m_target.draw(d, states);
    }

    /// Draws in screen pixels, ignoring the camera.
    void drawScreen(sf::Drawable const& d, sf::RenderStates const& states = sf::RenderStates::Default)
    {
        m_target.draw(d, states);
    }

    [[nodiscard]] sf::RenderTarget& target()       { return m_target; }
    [[nodiscard]] Camera const&     camera() const { return m_camera; }
    [[nodiscard]] sf::Vector2f      size() const   { return sf::Vector2f(m_target.getSize()); }

private:
    sf::RenderTarget& m_target;
    Camera const&     m_camera;
};

} // namespace sml
