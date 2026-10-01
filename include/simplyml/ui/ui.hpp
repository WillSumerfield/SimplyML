#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string_view>

#include "simplyml/core/app.hpp"
#include "simplyml/core/event_bus.hpp"
#include "simplyml/ui/key_bindings.hpp"
#include "simplyml/ui/layout.hpp"
#include "simplyml/ui/theme.hpp"
#include "simplyml/ui/widget.hpp"

namespace sml
{

class Control;

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

    /// Key shortcut, run on key press when no widget took the key (e.g. a focused text field).
    /// Without an action it only documents a key handled elsewhere, for KeyBindings legends, and
    /// the key passes through. Rebinding a key replaces its binding.
    void bindKey(sf::Keyboard::Key key, std::string description, std::function<void()> action = {});
    /// `key` triggers the control with this id (press a button, flip a toggle, next option);
    /// the description defaults to the control's label.
    void bindKey(sf::Keyboard::Key key, Control const& control, std::string description = {});
    void bindKey(sf::Keyboard::Key key, std::string description, std::string controlId);
    void unbindKey(sf::Keyboard::Key key);
    [[nodiscard]] std::vector<KeyBinding> const& keyBindings() const { return m_keys; }

    /// Held by the UI thread for each frame and input event.
    [[nodiscard]] std::recursive_mutex& mutex() { return m_mutex; }

    [[nodiscard]] Theme&       theme()       { return m_theme; }
    [[nodiscard]] Theme const& theme() const { return m_theme; }

    /// Lays out, updates and draws onto `target` (what the App does each frame; also usable
    /// off-screen with a RenderTexture).
    void frame(sf::RenderTarget& target, double now, float dt, sf::Vector2f mouse);

private:
    [[nodiscard]] UiContext context(double now, float dt, sf::Vector2f mouse);
    bool handle(sf::Event const& event);
    void addBinding(KeyBinding binding);

    App&                    m_app;
    std::recursive_mutex    m_mutex;
    Theme                   m_theme;
    std::unique_ptr<Widget> m_root;
    std::vector<KeyBinding> m_keys;
    std::uint64_t           m_drawId = 0;
    SubscriptionId          m_eventId = 0;
};

} // namespace sml
