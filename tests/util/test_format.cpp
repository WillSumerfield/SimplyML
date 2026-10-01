#include <doctest/doctest.h>
#include <simplyml/util/format.hpp>

TEST_CASE("toString")
{
    CHECK(sml::toString(3.14159f) == "3.14");
    CHECK(sml::toString(3.14159, 4) == "3.1416");
    CHECK(sml::toString(2.0f, 0) == "2");
    CHECK(sml::toString(42) == "42");
    CHECK(sml::toString(1e300).size() > 300); // longer than the stack buffer
}

TEST_CASE("formatDuration")
{
    CHECK(sml::formatDuration(0) == "0 seconds");
    CHECK(sml::formatDuration(1) == "1 second");
    CHECK(sml::formatDuration(65) == "1 minute, 5 seconds");
    CHECK(sml::formatDuration(3600) == "1 hour, 0 minutes");
    CHECK(sml::formatDuration(93784) == "1 day, 2 hours");
    CHECK(sml::formatDuration(93784, 4) == "1 day, 2 hours, 3 minutes, 4 seconds");
    CHECK(sml::formatDuration(2 * 365 * 86400.0 + 86400) == "2 years, 1 day");
    CHECK(sml::formatDuration(59.9, 1) == "59 seconds");
    CHECK(sml::formatDuration(-5) == "0 seconds");
}
