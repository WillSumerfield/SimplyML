#include <doctest/doctest.h>
#include <simplyml/core/control_store.hpp>

#include <thread>

TEST_CASE("ControlStore: set/get/declare/versions")
{
    sml::ControlStore c;
    CHECK_FALSE(c.get("lr"));
    CHECK(c.get("lr", 3.0) == 3.0);

    CHECK(c.declare("lr", 0.1) == 0.1);
    CHECK(c.declare("lr", 0.5) == 0.1); // exists: kept
    auto const v1 = c.entry("lr")->version;
    auto const v2 = c.set("lr", 0.2);
    CHECK(v2 > v1);
    CHECK(c.entry("lr")->version == v2);
    CHECK(*c.get("lr") == 0.2);
    CHECK(c.contains("lr"));

    c.set("a", 1.0);
    auto const all = c.all();
    REQUIRE(all.size() == 2);
    CHECK(all[0].first == "a");
    CHECK(all[1].second == 0.2);
}

TEST_CASE("ControlStore: concurrent writers")
{
    sml::ControlStore c;
    std::thread t{[&] {
        for (int i = 0; i < 10000; ++i) {
            c.set("x", i);
        }
    }};
    for (int i = 0; i < 10000; ++i) {
        (void)c.get("x");
    }
    t.join();
    CHECK(*c.get("x") == 9999.0);
}
