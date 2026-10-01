#include <doctest/doctest.h>
#include <simplyml/util/ring_buffer.hpp>

TEST_CASE("empty buffer is safe")
{
    sml::RingBuffer<float> rb{4};
    CHECK(rb.empty());
    CHECK(rb.mean() == 0.0f);
    CHECK(rb.diff() == 0.0f);
}

TEST_CASE("push, wrap, order")
{
    sml::RingBuffer<float> rb{3};
    rb.push(1);
    rb.push(2);
    CHECK(rb.size() == 2);
    CHECK(rb.front() == 1.0f);
    CHECK(rb.back() == 2.0f);
    CHECK(rb.mean() == doctest::Approx(1.5f));

    rb.push(3);
    rb.push(4);
    CHECK(rb.full());
    CHECK(rb.size() == 3);
    CHECK(rb[0] == 2.0f);
    CHECK(rb[1] == 3.0f);
    CHECK(rb[2] == 4.0f);
    CHECK(rb.sum() == doctest::Approx(9.0f));
    CHECK(rb.diff() == doctest::Approx(2.0f));

    float expected = 2.0f;
    rb.forEach([&](std::size_t, float v) { CHECK(v == expected++); });
}

TEST_CASE("clear resets the sum")
{
    sml::RingBuffer<float> rb{3};
    rb.push(10);
    rb.push(20);
    rb.clear();
    rb.push(1);
    CHECK(rb.mean() == doctest::Approx(1.0f));
    CHECK(rb.size() == 1);
}

TEST_CASE("setCapacity keeps newest values")
{
    sml::RingBuffer<int> rb{5};
    for (int i = 0; i < 7; ++i) {
        rb.push(i); // holds 2..6
    }
    rb.setCapacity(3);
    CHECK(rb.size() == 3);
    CHECK(rb.front() == 4);
    CHECK(rb.back() == 6);
    CHECK(rb.sum() == 15);
    rb.push(7);
    CHECK(rb.front() == 5);
    CHECK(rb.sum() == 18);

    rb.setCapacity(10);
    CHECK(rb.size() == 3);
    rb.push(8);
    CHECK(rb.front() == 5);
    CHECK(rb.back() == 8);
    CHECK(rb.sum() == 26);
}

TEST_CASE("running sum does not drift")
{
    sml::RingBuffer<float> rb{100};
    for (int i = 0; i < 1'000'000; ++i) {
        rb.push(i % 2 ? 1e6f : 1e-3f);
    }
    double exact = 0;
    rb.forEach([&](std::size_t, float v) { exact += v; });
    CHECK(rb.sum() == doctest::Approx(exact).epsilon(1e-5));
}
