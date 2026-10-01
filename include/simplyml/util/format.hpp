#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>

namespace sml
{

/// Fixed-point text for floats (`decimals` digits), plain text for integers.
template<typename T>
std::string toString(T v, int decimals = 2)
{
    static_assert(std::is_arithmetic_v<T>);
    if constexpr (std::is_integral_v<T>) {
        return std::to_string(v);
    } else {
        char buf[64];
        int const n = std::snprintf(buf, sizeof(buf), "%.*f", decimals, static_cast<double>(v));
        if (n < static_cast<int>(sizeof(buf))) {
            return {buf, static_cast<std::size_t>(n)};
        }
        std::string s(static_cast<std::size_t>(n), '\0');
        std::snprintf(s.data(), s.size() + 1, "%.*f", decimals, static_cast<double>(v));
        return s;
    }
}

/// Human duration using the `units` largest units, starting at the first non-zero one.
/// e.g. 93784 s -> "1 day, 2 hours"; 65 s -> "1 minute, 5 seconds"; 0 s -> "0 seconds".
inline std::string formatDuration(double seconds, int units = 2)
{
    struct Unit { char const* name; std::uint64_t secs; };
    static constexpr Unit table[] = {
        {"year", 365ull * 86400}, {"day", 86400}, {"hour", 3600}, {"minute", 60}, {"second", 1},
    };
    constexpr int count = static_cast<int>(sizeof(table) / sizeof(table[0]));

    std::uint64_t rem = seconds > 0.0 ? static_cast<std::uint64_t>(seconds) : 0;
    int first = 0;
    while (first < count - 1 && rem < table[first].secs) {
        ++first;
    }

    std::string out;
    int const last = std::min(first + std::max(units, 1), count);
    for (int i = first; i < last; ++i) {
        std::uint64_t const n = rem / table[i].secs;
        rem -= n * table[i].secs;
        if (!out.empty()) {
            out += ", ";
        }
        out += std::to_string(n);
        out += ' ';
        out += table[i].name;
        if (n != 1) {
            out += 's';
        }
    }
    return out;
}

} // namespace sml
