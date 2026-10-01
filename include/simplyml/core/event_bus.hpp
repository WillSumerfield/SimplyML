#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <SFML/Window/Event.hpp>

namespace sml
{

using SubscriptionId = std::uint64_t;

/// Plain, thread-portable form of an input or widget event, queued for consumers off the UI thread
/// (e.g. Python's `app.poll_events()`).
struct AppEvent
{
    enum class Type
    {
        Closed,
        KeyPressed,
        KeyReleased,
        MousePressed,
        MouseReleased,
        Custom, // posted by widgets/app code, identified by `name`
    };

    Type              type     = Type::Custom;
    sf::Keyboard::Key key      = sf::Keyboard::Key::Unknown;
    sf::Mouse::Button button   = sf::Mouse::Button::Left;
    sf::Vector2i      position = {};
    std::string       name     = {};
    double            value    = 0.0; // custom events from controls: the new value
};

/// Dispatches window events to any number of subscribers, highest priority first (ties in
/// subscription order). A handler returning true consumes the event and stops propagation;
/// handlers returning void never consume.
///
/// Subscribing/unsubscribing is thread-safe and allowed from inside handlers. Unconsumed input
/// events can also be queued for polling from another thread (`setQueueEnabled`).
class EventBus
{
public:
    using Handler = std::function<bool(sf::Event const&)>;

    /// Every event.
    template<typename F>
    SubscriptionId subscribe(F&& f, int priority = 0)
    {
        return add(wrap<sf::Event>(std::forward<F>(f)), priority);
    }

    /// One event subtype, e.g. `on<sf::Event::Resized>(...)`. `f` takes `T const&` or nothing.
    template<typename T, typename F>
    SubscriptionId on(F&& f, int priority = 0)
    {
        auto g = wrap<T>(std::forward<F>(f));
        return add([g = std::move(g)](sf::Event const& e) {
            auto const* t = e.getIf<T>();
            return t && g(*t);
        }, priority);
    }

    template<typename F>
    SubscriptionId onKeyPressed(sf::Keyboard::Key key, F&& f, int priority = 0)
    {
        return onMatch<sf::Event::KeyPressed>([key](auto const& e) { return e.code == key; }, std::forward<F>(f), priority);
    }

    template<typename F>
    SubscriptionId onKeyReleased(sf::Keyboard::Key key, F&& f, int priority = 0)
    {
        return onMatch<sf::Event::KeyReleased>([key](auto const& e) { return e.code == key; }, std::forward<F>(f), priority);
    }

    template<typename F>
    SubscriptionId onMousePressed(sf::Mouse::Button button, F&& f, int priority = 0)
    {
        return onMatch<sf::Event::MouseButtonPressed>([button](auto const& e) { return e.button == button; }, std::forward<F>(f), priority);
    }

    template<typename F>
    SubscriptionId onMouseReleased(sf::Mouse::Button button, F&& f, int priority = 0)
    {
        return onMatch<sf::Event::MouseButtonReleased>([button](auto const& e) { return e.button == button; }, std::forward<F>(f), priority);
    }

    void unsubscribe(SubscriptionId id);
    [[nodiscard]] std::size_t subscriberCount() const;

    /// Runs handlers in priority order until one consumes. Queues the event if unconsumed and
    /// queueing is enabled. Returns whether it was consumed.
    bool dispatch(sf::Event const& event);

    // Outbound queue ------------------------------------------------------------------------------
    /// Off by default so nothing piles up when nobody polls.
    void setQueueEnabled(bool enabled);
    /// Oldest events are dropped beyond this many (default 4096).
    void setQueueCapacity(std::size_t capacity);
    /// Queues an event directly (any thread); ignored while queueing is disabled.
    void post(AppEvent event);
    /// Takes all queued events, oldest first (any thread).
    std::vector<AppEvent> drain();

private:
    struct Entry
    {
        SubscriptionId    id       = 0;
        int               priority = 0;
        Handler           fn;
        std::atomic<bool> alive{true}; // cleared on unsubscribe; a dispatch holding an old list skips it
    };
    using List = std::vector<std::shared_ptr<Entry>>;

    template<typename T, typename F>
    static std::function<bool(T const&)> wrap(F&& f)
    {
        using Fn = std::decay_t<F>;
        return [f = Fn(std::forward<F>(f))](T const& t) mutable -> bool {
            if constexpr (std::is_invocable_v<Fn&, T const&>) {
                if constexpr (std::is_void_v<std::invoke_result_t<Fn&, T const&>>) {
                    f(t);
                    return false;
                } else {
                    return static_cast<bool>(f(t));
                }
            } else {
                if constexpr (std::is_void_v<std::invoke_result_t<Fn&>>) {
                    f();
                    return false;
                } else {
                    return static_cast<bool>(f());
                }
            }
        };
    }

    template<typename T, typename M, typename F>
    SubscriptionId onMatch(M match, F&& f, int priority)
    {
        auto g = wrap<T>(std::forward<F>(f));
        return add([match, g = std::move(g)](sf::Event const& e) {
            auto const* t = e.getIf<T>();
            return t && match(*t) && g(*t);
        }, priority);
    }

    SubscriptionId add(Handler fn, int priority);
    void           enqueue(AppEvent event);

    mutable std::mutex          m_mutex;
    std::shared_ptr<List const> m_list = std::make_shared<List const>();
    SubscriptionId              m_nextId = 1;

    std::mutex           m_queueMutex;
    std::deque<AppEvent> m_queue;
    std::size_t          m_queueCapacity = 4096;
    bool                 m_queueEnabled  = false;
};

} // namespace sml
