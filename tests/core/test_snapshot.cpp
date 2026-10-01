#include <doctest/doctest.h>
#include <simplyml/core/snapshot.hpp>

#include <atomic>
#include <thread>

namespace
{

struct Stats
{
    int    a = 0;
    double b = 0.0;
};

} // namespace

TEST_CASE("Snapshot: set/update/get/load")
{
    sml::Snapshot<Stats> snap{{1, 2.0}};
    CHECK(snap.get().a == 1);
    CHECK(snap.version() == 0);
    snap.set({3, 4.0});
    CHECK(snap.version() == 1);
    CHECK(snap.get().b == 4.0);
    snap.update([](Stats& s) { ++s.a; });
    CHECK(snap.load().a == 4);
    CHECK(snap.get().a == 4);
}

TEST_CASE("Snapshot: reader always sees a consistent value")
{
    sml::Snapshot<Stats> snap;
    std::atomic<bool> stop{false};
    std::thread writer([&] {
        for (int i = 0; i < 100000; ++i) {
            snap.set({i, static_cast<double>(i)});
        }
        stop = true;
    });
    bool consistent = true;
    while (!stop) {
        auto const& s = snap.get();
        consistent &= static_cast<double>(s.a) == s.b;
    }
    writer.join();
    CHECK(consistent);
    CHECK(snap.get().a == 99999);
}
