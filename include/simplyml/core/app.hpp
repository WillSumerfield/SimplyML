#pragma once
#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include "simplyml/core/camera.hpp"
#include "simplyml/core/canvas.hpp"
#include "simplyml/core/clock.hpp"
#include "simplyml/core/control_store.hpp"
#include "simplyml/core/event_bus.hpp"
#include "simplyml/core/metric_store.hpp"
#include "simplyml/core/resources.hpp"

namespace sf
{
class RenderWindow;
}

namespace sml
{

struct AppConfig
{
    std::string  title          = "SimplyML";
    sf::Vector2u size           = {1600, 900};
    bool         fullscreen     = false;
    unsigned     fpsLimit       = 60; // 0 = unlimited
    unsigned     antialiasing   = 4;
    bool         escToQuit      = false;
    bool         cameraControls = true; // drag to pan, wheel to zoom
    sf::Color    clearColor     = {80, 80, 80}; // Theme palette.clear
};

/// Owns the window, render loop, camera, resources, events and metric store. No global state:
/// any number of Apps may exist, each running at most one loop at a time.
///
/// Two run modes:
/// - `run(update)` blocks on the calling thread until the window closes (C++ apps).
/// - `start(update)` runs the same loop on a background thread (Python, threaded training).
///   The window, GL context and event polling all live on that thread.
///
/// Thread rules: `close`, `join`, `isRunning`, `post`, `setFpsLimit`, `setFullscreen`, `store()`
/// writers, `controls()` and `events()` subscribe/drain are safe from any thread. Everything else (camera,
/// resources, draw callbacks, window) belongs to the UI thread; reach it from elsewhere with
/// `post`, or configure it before starting.
class App
{
public:
    using UpdateFn = std::function<void(App&, float dt)>;
    using DrawFn   = std::function<void(Canvas&)>;
    using Task     = std::function<void(App&)>;

    explicit App(AppConfig config = {});
    ~App(); // closes and joins
    App(App const&)            = delete;
    App& operator=(App const&) = delete;

    void run(UpdateFn update = {});
    void start(UpdateFn update = {});
    /// Asks the loop to exit after the current frame.
    void close();
    /// Waits for a background loop; rethrows anything it threw.
    void join();
    [[nodiscard]] bool isRunning() const { return m_running.load(); }

    /// Runs `task` on the UI thread at the start of the next frame (or the first one, if not running).
    void post(Task task);
    /// Draw callbacks run in order each frame, after update. UI thread, or before starting.
    std::uint64_t onDraw(DrawFn draw);
    void          removeDraw(std::uint64_t id);

    void setFpsLimit(unsigned fps);
    /// Toggles between the configured limit and unlimited.
    void toggleFpsLimit();
    [[nodiscard]] bool isFpsLimited() const { return m_fpsLimit.load() != 0; }
    void setFullscreen(bool fullscreen);

    [[nodiscard]] AppConfig const&   config() const { return m_config; }
    [[nodiscard]] EventBus&          events()       { return m_events; }
    [[nodiscard]] Camera&            camera()       { return m_camera; }
    [[nodiscard]] Resources&         resources()    { return m_resources; }
    [[nodiscard]] MetricStore&       store()        { return m_store; }
    [[nodiscard]] ControlStore&      controls()     { return m_controls; }
    [[nodiscard]] Clock const&       clock() const  { return m_clock; }
    /// UI thread, while running; nullptr otherwise.
    [[nodiscard]] sf::RenderWindow*  window()       { return m_window.get(); }
    [[nodiscard]] sf::Vector2f       mouseScreen() const { return m_mouse; }
    [[nodiscard]] sf::Vector2f       mouseWorld() const  { return m_camera.screenToWorld(m_mouse); }

    /// Priority of the built-in camera controls: below widgets (0) so they capture the mouse first.
    static constexpr int CameraPriority = -1000;
    static constexpr int QuitPriority   = -2000;

private:
    void loop(UpdateFn const& update);
    void createWindow();
    void destroyWindow();
    void handle(sf::Event const& event);
    void runTasks();

    AppConfig   m_config;
    EventBus    m_events;
    Camera      m_camera;
    Resources   m_resources;
    MetricStore m_store;
    ControlStore m_controls;
    Clock       m_clock;

    std::unique_ptr<sf::RenderWindow> m_window;
    sf::Vector2f                      m_mouse;
    std::vector<std::pair<std::uint64_t, DrawFn>> m_draws;
    std::uint64_t                     m_nextDraw = 1;

    std::mutex        m_taskMutex;
    std::vector<Task> m_tasks;

    std::atomic<bool>     m_running{false};
    std::atomic<bool>     m_closeRequested{false};
    std::atomic<unsigned> m_fpsLimit;
    std::atomic<bool>     m_fullscreen;
    unsigned              m_appliedFps    = 0;
    bool                  m_appliedFull   = false;
    std::thread           m_thread;
    std::exception_ptr    m_error;
};

} // namespace sml
