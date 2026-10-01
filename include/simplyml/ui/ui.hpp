#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string_view>

#include "simplyml/core/app.hpp"
#include "simplyml/core/event_bus.hpp"
#include "simplyml/ui/layout.hpp"
#include "simplyml/ui/theme.hpp"
#include "simplyml/ui/widget.hpp"

namespace sml
{

/// The widget tree of one App: lays it out to the window (on resize and every frame), updates
/// and draws it in screen space, and feeds it input at priority 0 (above the camera controls).
///
/// Build the tree before `App::start`, from the UI thread (`App::post`), or from any thread while
/// holding `mutex()` (the UI thread holds it while laying out, drawing and dispatching input).
/// Destroy the Ui only while the App's loop isn't running.
class Ui
{
public:
    explicit Ui(App& app, Theme theme = {});
    ~Ui();
    Ui(Ui const&)            = delete;
    Ui& operator=(Ui const&) = delete;

    /// Replaces the root widget (default: an empty Column) and returns it.
    template<typename T, typename... Args>
    T& setRoot(Args&&... args)
    {
        auto w = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *w;
        m_root = std::move(w);
        return ref;
    }
    [[nodiscard]] Widget& root() { return *m_root; }

    /// Widget with `id`, else nullptr / `std::out_of_range`.
    [[nodiscard]] Widget* find(std::string_view id) { return m_root->find(id); }
    [[nodiscard]] Widget& operator[](std::string_view id);
    template<typename T>
    [[nodiscard]] T& get(std::string_view id)
    {
        auto* w = dynamic_cast<T*>(find(id));
        if (!w) {
            throw std::out_of_range("SimplyML: no widget of that type with id '" + std::string(id) + "'");
        }
        return *w;
    }

    /// Held by the UI thread for each frame and input event.
    [[nodiscard]] std::recursive_mutex& mutex() { return m_mutex; }

    [[nodiscard]] Theme&       theme()       { return m_theme; }
    [[nodiscard]] Theme const& theme() const { return m_theme; }

    /// Lays out, updates and draws onto `target` (what the App does each frame; also usable
    /// off-screen with a RenderTexture).
    void frame(sf::RenderTarget& target, double now, float dt, sf::Vector2f mouse);

private:
    [[nodiscard]] UiContext context(double now, float dt, sf::Vector2f mouse) const;

    App&                    m_app;
    std::recursive_mutex    m_mutex;
    Theme                   m_theme;
    std::unique_ptr<Widget> m_root;
    std::uint64_t           m_drawId = 0;
    SubscriptionId          m_eventId = 0;
};

} // namespace sml
