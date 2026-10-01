#include "simplyml/core/app.hpp"

#include <algorithm>
#include <stdexcept>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>

namespace sml
{

namespace
{

// SFML's GL setup (shared context, GLX extension loading) races when several threads create or
// destroy windows at once, so Apps on different threads take turns.
std::mutex& windowMutex()
{
    static std::mutex m;
    return m;
}

} // namespace

App::App(AppConfig config)
    : m_config{std::move(config)}
    , m_camera{sf::Vector2f(m_config.size)}
    , m_fpsLimit{m_config.fpsLimit}
    , m_fullscreen{m_config.fullscreen}
{
    if (m_config.cameraControls) {
        m_events.subscribe([this](sf::Event const& e) { return m_camera.handle(e); }, CameraPriority);
    }
    if (m_config.escToQuit) {
        m_events.onKeyPressed(sf::Keyboard::Key::Escape, [this] { close(); return true; }, QuitPriority);
    }
}

App::~App()
{
    close();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void App::run(UpdateFn update)
{
    if (m_running.exchange(true)) {
        throw std::logic_error("SimplyML: App is already running");
    }
    m_closeRequested = false;
    try {
        loop(update);
    } catch (...) {
        destroyWindow();
        m_running = false;
        throw;
    }
    m_running = false;
}

void App::start(UpdateFn update)
{
    if (m_running.exchange(true)) {
        throw std::logic_error("SimplyML: App is already running");
    }
    if (m_thread.joinable()) {
        m_thread.join(); // previous, finished loop
    }
    m_closeRequested = false;
    m_error          = nullptr;
    m_thread         = std::thread([this, update = std::move(update)] {
        try {
            loop(update);
        } catch (...) {
            destroyWindow();
            m_error = std::current_exception();
        }
        m_running = false;
    });
}

void App::close()
{
    m_closeRequested = true;
}

void App::join()
{
    if (m_thread.joinable()) {
        m_thread.join();
    }
    if (m_error) {
        std::rethrow_exception(std::exchange(m_error, nullptr));
    }
}

void App::post(Task task)
{
    std::lock_guard lock{m_taskMutex};
    m_tasks.push_back(std::move(task));
}

std::uint64_t App::onDraw(DrawFn draw)
{
    m_draws.emplace_back(m_nextDraw, std::move(draw));
    return m_nextDraw++;
}

void App::removeDraw(std::uint64_t id)
{
    m_draws.erase(std::remove_if(m_draws.begin(), m_draws.end(), [id](auto const& d) { return d.first == id; }),
                  m_draws.end());
}

void App::setFpsLimit(unsigned fps)
{
    m_fpsLimit = fps;
}

void App::toggleFpsLimit()
{
    m_fpsLimit = m_fpsLimit.load() ? 0u : m_config.fpsLimit;
}

void App::setFullscreen(bool fullscreen)
{
    m_fullscreen = fullscreen;
}

void App::createWindow()
{
    sf::ContextSettings settings;
    settings.antiAliasingLevel = m_config.antialiasing;

    std::lock_guard lock{windowMutex()};
    m_window.reset();
    m_appliedFull = m_fullscreen.load();
    if (m_appliedFull) {
        m_window = std::make_unique<sf::RenderWindow>(sf::VideoMode::getDesktopMode(), m_config.title,
            sf::State::Fullscreen, settings);
    } else {
        m_window = std::make_unique<sf::RenderWindow>(sf::VideoMode(m_config.size), m_config.title,
            sf::Style::Default, sf::State::Windowed, settings);
    }
    m_appliedFps = m_fpsLimit.load();
    m_window->setFramerateLimit(m_appliedFps);

    sf::Vector2f const size{m_window->getSize()};
    m_window->setView(sf::View(sf::FloatRect({0.0f, 0.0f}, size)));
    m_camera.setViewSize(size);
}

void App::runTasks()
{
    std::vector<Task> tasks;
    {
        std::lock_guard lock{m_taskMutex};
        tasks.swap(m_tasks);
    }
    for (auto& t : tasks) {
        t(*this);
    }
}

void App::handle(sf::Event const& event)
{
    if (auto const* r = event.getIf<sf::Event::Resized>()) {
        sf::Vector2f const size{r->size};
        m_window->setView(sf::View(sf::FloatRect({0.0f, 0.0f}, size)));
        m_camera.setViewSize(size);
    } else if (auto const* m = event.getIf<sf::Event::MouseMoved>()) {
        m_mouse = sf::Vector2f(m->position);
    }
    m_events.dispatch(event);
    if (event.is<sf::Event::Closed>()) {
        close();
    }
}

void App::loop(UpdateFn const& update)
{
    createWindow();
    m_clock.tick();
    while (!m_closeRequested) {
        runTasks();

        if (m_fullscreen.load() != m_appliedFull) {
            createWindow();
        }
        if (unsigned const fps = m_fpsLimit.load(); fps != m_appliedFps) {
            m_appliedFps = fps;
            m_window->setFramerateLimit(fps);
        }

        while (auto const event = m_window->pollEvent()) {
            handle(*event);
        }

        float const dt = m_clock.tick();
        m_store.sync();
        if (update) {
            update(*this, dt);
        }

        m_window->clear(m_config.clearColor);
        Canvas canvas{*m_window, m_camera};
        for (std::size_t i = 0; i < m_draws.size(); ++i) { // index: a callback may add/remove draws
            auto const fn = m_draws[i].second;
            fn(canvas);
        }
        m_window->display();
    }
    m_store.sync(); // dumps after close see every point pushed so far
    destroyWindow();
}

void App::destroyWindow()
{
    std::lock_guard lock{windowMutex()};
    m_window.reset();
}

} // namespace sml
