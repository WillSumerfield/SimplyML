#include "simplyml/core/event_bus.hpp"

#include <algorithm>
#include <optional>

namespace sml
{

namespace
{

std::optional<AppEvent> toAppEvent(sf::Event const& e)
{
    using T = AppEvent::Type;
    if (e.is<sf::Event::Closed>()) {
        return AppEvent{T::Closed};
    }
    if (auto const* k = e.getIf<sf::Event::KeyPressed>()) {
        return AppEvent{T::KeyPressed, k->code};
    }
    if (auto const* k = e.getIf<sf::Event::KeyReleased>()) {
        return AppEvent{T::KeyReleased, k->code};
    }
    if (auto const* m = e.getIf<sf::Event::MouseButtonPressed>()) {
        return AppEvent{T::MousePressed, sf::Keyboard::Key::Unknown, m->button, m->position};
    }
    if (auto const* m = e.getIf<sf::Event::MouseButtonReleased>()) {
        return AppEvent{T::MouseReleased, sf::Keyboard::Key::Unknown, m->button, m->position};
    }
    return std::nullopt;
}

} // namespace

SubscriptionId EventBus::add(Handler fn, int priority)
{
    std::lock_guard lock{m_mutex};
    auto entry      = std::make_shared<Entry>();
    entry->id       = m_nextId++;
    entry->priority = priority;
    entry->fn       = std::move(fn);
    SubscriptionId const id = entry->id;
    auto list  = std::make_shared<List>(*m_list);
    // after all entries with priority >= new one: stable among equals
    auto const pos = std::upper_bound(list->begin(), list->end(), priority,
        [](int p, std::shared_ptr<Entry> const& e) { return p > e->priority; });
    list->insert(pos, std::move(entry));
    m_list = std::move(list);
    return id;
}

void EventBus::unsubscribe(SubscriptionId id)
{
    std::lock_guard lock{m_mutex};
    auto list = std::make_shared<List>(*m_list);
    auto const it = std::find_if(list->begin(), list->end(), [id](auto const& e) { return e->id == id; });
    if (it == list->end()) {
        return;
    }
    (*it)->alive = false;
    list->erase(it);
    m_list = std::move(list);
}

std::size_t EventBus::subscriberCount() const
{
    std::lock_guard lock{m_mutex};
    return m_list->size();
}

bool EventBus::dispatch(sf::Event const& event)
{
    std::shared_ptr<List const> list;
    {
        std::lock_guard lock{m_mutex};
        list = m_list;
    }
    bool consumed = false;
    for (auto const& entry : *list) {
        if (entry->alive && entry->fn(event)) {
            consumed = true;
            break;
        }
    }
    if (!consumed) {
        if (auto app = toAppEvent(event)) {
            enqueue(std::move(*app));
        }
    }
    return consumed;
}

void EventBus::setQueueEnabled(bool enabled)
{
    std::lock_guard lock{m_queueMutex};
    m_queueEnabled = enabled;
    if (!enabled) {
        m_queue.clear();
    }
}

void EventBus::setQueueCapacity(std::size_t capacity)
{
    std::lock_guard lock{m_queueMutex};
    m_queueCapacity = std::max<std::size_t>(capacity, 1);
    while (m_queue.size() > m_queueCapacity) {
        m_queue.pop_front();
    }
}

void EventBus::post(AppEvent event)
{
    enqueue(std::move(event));
}

void EventBus::enqueue(AppEvent event)
{
    std::lock_guard lock{m_queueMutex};
    if (!m_queueEnabled) {
        return;
    }
    if (m_queue.size() >= m_queueCapacity) {
        m_queue.pop_front();
    }
    m_queue.push_back(std::move(event));
}

std::vector<AppEvent> EventBus::drain()
{
    std::lock_guard lock{m_queueMutex};
    std::vector<AppEvent> out(std::make_move_iterator(m_queue.begin()), std::make_move_iterator(m_queue.end()));
    m_queue.clear();
    return out;
}

} // namespace sml
