#include <doctest/doctest.h>
#include <simplyml/core/metric_store.hpp>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

TEST_CASE("MetricStore: pushes are invisible until sync")
{
    sml::MetricStore store;
    store.push("loss", 0.0, 1.0);
    CHECK(store.series("loss") == nullptr);
    CHECK(store.sync());
    auto const* s = store.series("loss");
    REQUIRE(s);
    CHECK(s->size() == 1);
    CHECK(s->last().value == 1.0);
    CHECK_FALSE(store.sync()); // nothing new
}

TEST_CASE("MetricStore: auto step continues from last step")
{
    sml::MetricStore store;
    store.push("a", 5.0);
    store.push("a", 6.0);
    store.push("a", 10.0, 0.0);
    store.push("a", 7.0);
    store.sync();
    auto const& p = store.series("a")->points();
    REQUIRE(p.size() == 4);
    CHECK(p[0].step == 0.0);
    CHECK(p[1].step == 1.0);
    CHECK(p[3].step == 11.0);
}

TEST_CASE("MetricStore: pushMany, maxPoints and total")
{
    sml::MetricStore store;
    std::vector<double> steps{0, 1, 2, 3, 4};
    std::vector<double> vals{10, 11, 12, 13, 14};
    store.setMaxPoints("x", 3);
    store.pushMany("x", steps.data(), vals.data(), steps.size());
    store.sync();
    auto const* s = store.series("x");
    REQUIRE(s);
    CHECK(s->size() == 3);
    CHECK(s->total() == 5);
    CHECK(s->points().front().value == 12.0);
    CHECK(s->maxPoints() == 3);
}

TEST_CASE("MetricStore: clear drops synced and pending data")
{
    sml::MetricStore store;
    store.push("a", 1.0);
    store.sync();
    store.push("a", 2.0);
    store.clear();
    store.push("b", 3.0);
    CHECK(store.sync());
    CHECK(store.series("a") == nullptr);
    REQUIRE(store.series("b"));
    CHECK(store.series("b")->last().step == 0.0);
    CHECK(store.names() == std::vector<std::string>{"b"});
}

TEST_CASE("MetricStore: concurrent writers while the reader syncs")
{
    sml::MetricStore store;
    constexpr int kThreads = 4;
    constexpr int kPushes  = 20000;
    std::atomic<int> done{0};
    std::vector<std::thread> writers;
    for (int t = 0; t < kThreads; ++t) {
        writers.emplace_back([&, t] {
            std::string const name = t % 2 ? "odd" : "even";
            for (int i = 0; i < kPushes; ++i) {
                store.push(name, i, t);
            }
            ++done;
        });
    }
    while (done < kThreads) {
        store.sync();
    }
    for (auto& w : writers) {
        w.join();
    }
    store.sync();
    CHECK(store.series("odd")->size() == 2 * kPushes);
    CHECK(store.series("even")->total() == 2 * kPushes);
}

TEST_CASE("MetricStore: CSV and binary dumps")
{
    namespace fs = std::filesystem;
    sml::MetricStore store;
    store.push("loss", 0.0, 0.5);
    store.push("loss", 1.0, 0.25);
    store.push("acc", 3.0, 0.125);
    store.sync();

    auto const dir = fs::temp_directory_path() / "simplyml_test_metric_store";
    fs::create_directories(dir);

    store.writeCsv(dir / "m.csv");
    std::ifstream csv{dir / "m.csv"};
    std::stringstream ss;
    ss << csv.rdbuf();
    CHECK(ss.str() == "series,step,value\nacc,3,0.125\nloss,0,0.5\nloss,1,0.25\n");

    store.writeBinary(dir / "m.bin");
    sml::MetricStore loaded;
    loaded.readBinary(dir / "m.bin");
    REQUIRE(loaded.series("loss"));
    CHECK(loaded.series("loss")->size() == 2);
    CHECK(loaded.series("loss")->last().value == 0.25);
    CHECK(loaded.series("acc")->last().step == 3.0);

    std::ofstream{dir / "junk.bin"} << "nope";
    CHECK_THROWS(loaded.readBinary(dir / "junk.bin"));
    CHECK(loaded.series("acc")); // untouched on failure
    fs::remove_all(dir);
}
