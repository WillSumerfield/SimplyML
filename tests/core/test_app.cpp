// Needs a display; skipped on headless Linux.
#include <doctest/doctest.h>
#include <simplyml/core/app.hpp>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <thread>

namespace
{

bool hasDisplay()
{
#if defined(__linux__)
    return std::getenv("DISPLAY") || std::getenv("WAYLAND_DISPLAY");
#else
    return true;
#endif
}

sml::AppConfig smallWindow()
{
    sml::AppConfig c;
    c.title    = "SimplyML test";
    c.size     = {320, 240};
    c.fpsLimit = 0;
    return c;
}

} // namespace

TEST_CASE("App: run blocks on this thread and draws" * doctest::skip(!hasDisplay()))
{
    sml::App app{smallWindow()};
    int draws = 0;
    app.onDraw([&](sml::Canvas& c) {
        sf::RectangleShape r{{10.0f, 10.0f}};
        c.drawWorld(r);
        c.drawScreen(r);
        ++draws;
    });
    int updates = 0;
    app.run([&](sml::App& a, float dt) {
        CHECK(dt >= 0.0f);
        CHECK(a.window() != nullptr);
        if (++updates == 5) {
            a.close();
        }
    });
    CHECK(updates == 5);
    CHECK(draws == 5);
    CHECK_FALSE(app.isRunning());
    CHECK(app.window() == nullptr);
}

TEST_CASE("App: background thread renders while main thread pushes" * doctest::skip(!hasDisplay()))
{
    sml::App app{smallWindow()};
    std::atomic<int>         frames{0};
    std::atomic<std::size_t> seen{0};
    std::thread::id          uiThread;
    app.onDraw([&](sml::Canvas&) {
        uiThread = std::this_thread::get_id();
        if (auto const* s = app.store().series("loss")) {
            seen = s->size();
        }
        ++frames;
    });
    app.start();
    CHECK(app.isRunning());

    for (int i = 0; i < 500; ++i) {
        app.store().push("loss", i, 1.0 / (i + 1));
    }
    std::atomic<bool> posted{false};
    app.post([&](sml::App& a) {
        posted = a.window() != nullptr;
    });
    auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while ((seen < 500 || !posted || frames < 3) && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    app.close();
    app.join();
    CHECK(seen == 500);
    CHECK(posted);
    CHECK(uiThread != std::this_thread::get_id());
    CHECK_FALSE(app.isRunning());
}

TEST_CASE("App: errors on the UI thread surface in join" * doctest::skip(!hasDisplay()))
{
    sml::App app{smallWindow()};
    app.start([](sml::App&, float) { throw std::runtime_error("boom"); });
    CHECK_THROWS_WITH(app.join(), "boom");
    CHECK_FALSE(app.isRunning());
    CHECK_THROWS_AS(([&] { app.start(); app.start(); })(), std::logic_error);
    app.close();
    app.join();
}

TEST_CASE("App: two apps, no shared state" * doctest::skip(!hasDisplay()))
{
    sml::App a{smallWindow()};
    sml::App b{smallWindow()};
    a.store().push("x", 1.0);
    a.start();
    b.start();
    a.close();
    b.close();
    a.join();
    b.join();
    a.store().sync(); // closed before its first frame: sync by hand
    b.store().sync();
    CHECK(a.store().series("x"));
    CHECK(b.store().series("x") == nullptr);
}
