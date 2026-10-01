#include <doctest/doctest.h>
#include <simplyml/core/event_bus.hpp>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace
{

sf::Event key(sf::Keyboard::Key k)
{
    return sf::Event::KeyPressed{k, sf::Keyboard::Scancode::Unknown, false, false, false, false};
}

} // namespace

TEST_CASE("EventBus: two subscribers hear the same key")
{
    sml::EventBus bus;
    int a = 0;
    int b = 0;
    bus.onKeyPressed(sf::Keyboard::Key::S, [&] { ++a; });
    bus.onKeyPressed(sf::Keyboard::Key::S, [&] { ++b; });
    bus.onKeyPressed(sf::Keyboard::Key::D, [&] { a += 100; });
    CHECK_FALSE(bus.dispatch(key(sf::Keyboard::Key::S)));
    CHECK(a == 1);
    CHECK(b == 1);
}

TEST_CASE("EventBus: priority order and consumption")
{
    sml::EventBus bus;
    std::string order;
    bus.subscribe([&] { order += 'l'; }, -5);
    bus.subscribe([&] { order += 'a'; }, 0);
    bus.subscribe([&] { order += 'h'; return false; }, 10);
    bus.subscribe([&] { order += 'b'; }, 0); // same priority: registration order
    bus.dispatch(sf::Event::Closed{});
    CHECK(order == "habl");

    order.clear();
    bus.subscribe([&](sf::Event const&) { order += 'C'; return true; }, 1);
    CHECK(bus.dispatch(sf::Event::Closed{}));
    CHECK(order == "hC");
}

TEST_CASE("EventBus: typed subscriptions receive the subtype")
{
    sml::EventBus bus;
    sf::Vector2u got;
    bus.on<sf::Event::Resized>([&](sf::Event::Resized const& r) { got = r.size; });
    bus.dispatch(sf::Event::Resized{{640u, 480u}});
    bus.dispatch(sf::Event::Closed{});
    CHECK(got == sf::Vector2u{640u, 480u});

    int clicks = 0;
    bus.onMousePressed(sf::Mouse::Button::Right, [&](sf::Event::MouseButtonPressed const& e) { clicks += e.position.x; });
    bus.dispatch(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {1, 0}});
    bus.dispatch(sf::Event::MouseButtonPressed{sf::Mouse::Button::Right, {7, 0}});
    CHECK(clicks == 7);
}

TEST_CASE("EventBus: unsubscribe, including from inside a handler")
{
    sml::EventBus bus;
    int a = 0;
    int b = 0;
    sml::SubscriptionId idB = 0;
    auto const idA = bus.subscribe([&] { ++a; bus.unsubscribe(idB); }, 1);
    idB = bus.subscribe([&] { ++b; });
    bus.dispatch(sf::Event::Closed{});
    CHECK(a == 1);
    CHECK(b == 0); // removed mid-dispatch: skipped
    bus.unsubscribe(idA);
    bus.dispatch(sf::Event::Closed{});
    CHECK(a == 1);
    CHECK(bus.subscriberCount() == 0);
}

TEST_CASE("EventBus: queue gets unconsumed input only, when enabled, capped")
{
    sml::EventBus bus;
    bus.dispatch(key(sf::Keyboard::Key::A));
    CHECK(bus.drain().empty()); // disabled by default

    bus.setQueueEnabled(true);
    bus.onKeyPressed(sf::Keyboard::Key::B, [] { return true; });
    bus.dispatch(key(sf::Keyboard::Key::A));
    bus.dispatch(key(sf::Keyboard::Key::B)); // consumed
    bus.dispatch(sf::Event::MouseMoved{{1, 2}}); // not an AppEvent
    bus.post({sml::AppEvent::Type::Custom, sf::Keyboard::Key::Unknown, sf::Mouse::Button::Left, {}, "reset"});
    auto const q = bus.drain();
    REQUIRE(q.size() == 2);
    CHECK(q[0].type == sml::AppEvent::Type::KeyPressed);
    CHECK(q[0].key == sf::Keyboard::Key::A);
    CHECK(q[1].name == "reset");
    CHECK(bus.drain().empty());

    bus.setQueueCapacity(3);
    for (int i = 0; i < 5; ++i) {
        bus.post({sml::AppEvent::Type::Custom, sf::Keyboard::Key::Unknown, sf::Mouse::Button::Left, {}, std::to_string(i)});
    }
    auto const capped = bus.drain();
    REQUIRE(capped.size() == 3);
    CHECK(capped.front().name == "2");
}

TEST_CASE("EventBus: concurrent subscribe while dispatching")
{
    sml::EventBus bus;
    std::atomic<int> hits{0};
    std::atomic<bool> stop{false};
    std::thread dispatcher([&] {
        while (!stop) {
            bus.dispatch(sf::Event::Closed{});
        }
    });
    std::vector<sml::SubscriptionId> ids;
    for (int i = 0; i < 200; ++i) {
        ids.push_back(bus.subscribe([&] { ++hits; }));
    }
    for (auto id : ids) {
        bus.unsubscribe(id);
    }
    stop = true;
    dispatcher.join();
    CHECK(bus.subscriberCount() == 0);
}
