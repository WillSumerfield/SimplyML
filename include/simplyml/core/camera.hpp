#pragma once
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>

namespace sml
{

/// Maps world space to screen pixels: `focus` (world) sits at the view center, scaled by `zoom`.
/// The world origin starts at the view center. Optional mouse controls: drag to pan, wheel to zoom
/// toward the cursor.
class Camera
{
public:
    explicit Camera(sf::Vector2f viewSize = {}, sf::Vector2f focus = {}, float zoom = 1.0f);

    [[nodiscard]] sf::Vector2f worldToScreen(sf::Vector2f world) const;
    [[nodiscard]] sf::Vector2f screenToWorld(sf::Vector2f screen) const;
    [[nodiscard]] sf::Transform const& transform() const { return m_transform; }

    /// Moves the view by a screen-space delta (content follows the mouse).
    void pan(sf::Vector2f screenDelta);
    /// Multiplies zoom by `factor`, keeping the world point under `screenPos` fixed.
    void zoomAt(sf::Vector2f screenPos, float factor);
    /// Back to the focus/zoom given at construction.
    void reset();

    void setViewSize(sf::Vector2f size);
    void setFocus(sf::Vector2f world);
    void setZoom(float zoom);
    void setZoomLimits(float minZoom, float maxZoom);
    /// Zoom factor applied per wheel notch (default 1.2).
    void setZoomStep(float step) { m_zoomStep = step; }

    [[nodiscard]] sf::Vector2f viewSize() const { return m_viewSize; }
    [[nodiscard]] sf::Vector2f focus() const    { return m_focus; }
    [[nodiscard]] float        zoom() const     { return m_zoom; }

    /// Mouse controls (left drag pans, wheel zooms). Returns true if the event was consumed.
    bool handle(sf::Event const& event);
    [[nodiscard]] bool dragging() const { return m_dragging; }

private:
    void update();

    sf::Vector2f  m_viewSize;
    sf::Vector2f  m_focus;
    float         m_zoom;
    sf::Vector2f  m_initFocus;
    float         m_initZoom;
    float         m_minZoom  = 1e-4f;
    float         m_maxZoom  = 1e4f;
    float         m_zoomStep = 1.2f;
    bool          m_dragging = false;
    sf::Vector2f  m_lastMouse;
    sf::Transform m_transform;
};

} // namespace sml
