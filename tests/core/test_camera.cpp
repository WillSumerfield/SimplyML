#include <doctest/doctest.h>
#include <simplyml/core/camera.hpp>

namespace
{

void checkNear(sf::Vector2f a, sf::Vector2f b)
{
    CHECK(a.x == doctest::Approx(b.x).epsilon(1e-4));
    CHECK(a.y == doctest::Approx(b.y).epsilon(1e-4));
}

} // namespace

TEST_CASE("Camera: world origin starts at view center")
{
    sml::Camera cam{{800.0f, 600.0f}};
    checkNear(cam.worldToScreen({0.0f, 0.0f}), {400.0f, 300.0f});
    checkNear(cam.screenToWorld({400.0f, 300.0f}), {0.0f, 0.0f});
}

TEST_CASE("Camera: screen/world round trip and transform agree")
{
    sml::Camera cam{{800.0f, 600.0f}, {10.0f, -5.0f}, 2.5f};
    sf::Vector2f const w{37.0f, -12.0f};
    checkNear(cam.screenToWorld(cam.worldToScreen(w)), w);
    checkNear(cam.transform().transformPoint(w), cam.worldToScreen(w));
}

TEST_CASE("Camera: zoomAt keeps the point under the cursor fixed")
{
    sml::Camera cam{{800.0f, 600.0f}};
    sf::Vector2f const cursor{620.0f, 140.0f};
    sf::Vector2f const before = cam.screenToWorld(cursor);
    cam.zoomAt(cursor, 1.2f);
    cam.zoomAt(cursor, 1.2f);
    CHECK(cam.zoom() == doctest::Approx(1.44f));
    checkNear(cam.screenToWorld(cursor), before);
}

TEST_CASE("Camera: pan moves content with the mouse; reset restores construction state")
{
    sml::Camera cam{{800.0f, 600.0f}, {5.0f, 5.0f}, 2.0f};
    sf::Vector2f const w{1.0f, 2.0f};
    sf::Vector2f const s = cam.worldToScreen(w);
    cam.pan({30.0f, -10.0f});
    checkNear(cam.worldToScreen(w), s + sf::Vector2f{30.0f, -10.0f});
    cam.setZoom(7.0f);
    cam.reset();
    checkNear(cam.focus(), {5.0f, 5.0f});
    CHECK(cam.zoom() == 2.0f);
}

TEST_CASE("Camera: zoom limits clamp")
{
    sml::Camera cam{{100.0f, 100.0f}};
    cam.setZoomLimits(0.5f, 2.0f);
    cam.setZoom(10.0f);
    CHECK(cam.zoom() == 2.0f);
    cam.zoomAt({0.0f, 0.0f}, 0.01f);
    CHECK(cam.zoom() == 0.5f);
}

TEST_CASE("Camera: mouse controls")
{
    sml::Camera cam{{800.0f, 600.0f}};
    CHECK(cam.handle(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {100, 100}}));
    CHECK(cam.dragging());
    CHECK_FALSE(cam.handle(sf::Event::MouseMoved{{130, 90}})); // moves never consumed
    checkNear(cam.focus(), {-30.0f, 10.0f});
    CHECK(cam.handle(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {130, 90}}));
    CHECK_FALSE(cam.dragging());

    CHECK(cam.handle(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical, 1.0f, {400, 300}}));
    CHECK(cam.zoom() == doctest::Approx(1.2f));
    CHECK_FALSE(cam.handle(sf::Event::MouseButtonPressed{sf::Mouse::Button::Right, {0, 0}}));
}
