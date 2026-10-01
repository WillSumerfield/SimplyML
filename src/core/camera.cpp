#include "simplyml/core/camera.hpp"

#include <algorithm>

namespace sml
{

Camera::Camera(sf::Vector2f viewSize, sf::Vector2f focus, float zoom)
    : m_viewSize{viewSize}
    , m_focus{focus}
    , m_zoom{zoom}
    , m_initFocus{focus}
    , m_initZoom{zoom}
{
    update();
}

sf::Vector2f Camera::worldToScreen(sf::Vector2f world) const
{
    return (world - m_focus) * m_zoom + m_viewSize * 0.5f;
}

sf::Vector2f Camera::screenToWorld(sf::Vector2f screen) const
{
    return m_focus + (screen - m_viewSize * 0.5f) / m_zoom;
}

void Camera::pan(sf::Vector2f screenDelta)
{
    m_focus -= screenDelta / m_zoom;
    update();
}

void Camera::zoomAt(sf::Vector2f screenPos, float factor)
{
    sf::Vector2f const anchor = screenToWorld(screenPos);
    m_zoom  = std::clamp(m_zoom * factor, m_minZoom, m_maxZoom);
    m_focus = anchor - (screenPos - m_viewSize * 0.5f) / m_zoom;
    update();
}

void Camera::reset()
{
    m_focus = m_initFocus;
    m_zoom  = m_initZoom;
    update();
}

void Camera::setViewSize(sf::Vector2f size)
{
    m_viewSize = size;
    update();
}

void Camera::setFocus(sf::Vector2f world)
{
    m_focus = world;
    update();
}

void Camera::setZoom(float zoom)
{
    m_zoom = std::clamp(zoom, m_minZoom, m_maxZoom);
    update();
}

void Camera::setZoomLimits(float minZoom, float maxZoom)
{
    m_minZoom = minZoom;
    m_maxZoom = maxZoom;
    setZoom(m_zoom);
}

bool Camera::handle(sf::Event const& event)
{
    if (auto const* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Left) {
            m_dragging  = true;
            m_lastMouse = sf::Vector2f(e->position);
            return true;
        }
    } else if (auto const* e = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (e->button == sf::Mouse::Button::Left && m_dragging) {
            m_dragging = false;
            return true;
        }
    } else if (auto const* e = event.getIf<sf::Event::MouseMoved>()) {
        // never consumed: hover state elsewhere still needs mouse moves
        sf::Vector2f const p{e->position};
        if (m_dragging) {
            pan(p - m_lastMouse);
        }
        m_lastMouse = p;
    } else if (auto const* e = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (e->wheel == sf::Mouse::Wheel::Vertical && e->delta != 0.0f) {
            zoomAt(sf::Vector2f(e->position), e->delta > 0.0f ? m_zoomStep : 1.0f / m_zoomStep);
            return true;
        }
    }
    return false;
}

void Camera::update()
{
    m_transform = sf::Transform::Identity;
    m_transform.translate(m_viewSize * 0.5f);
    m_transform.scale({m_zoom, m_zoom});
    m_transform.translate(-m_focus);
}

} // namespace sml
