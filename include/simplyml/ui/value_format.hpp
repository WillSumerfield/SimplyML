#pragma once
#include <functional>
#include <string>

namespace sml
{

/// How a number is shown in a readout or label.
struct ValueFormat
{
    enum class Kind
    {
        Number,   // fixed decimals
        Duration, // seconds as "1 day, 2 hours"
        Percent,  // ratio 0..1 as "42.0%"
    };

    Kind        kind     = Kind::Number;
    int         decimals = 2;
    int         pad      = 0; // zero-pad the integer part to this many digits ("0042")
    std::string prefix;
    std::string suffix;
    /// Overrides everything above when set.
    std::function<std::string(double)> custom;

    [[nodiscard]] std::string operator()(double v) const;

    [[nodiscard]] static ValueFormat number(int decimals, std::string suffix = {})
    {
        ValueFormat f;
        f.decimals = decimals;
        f.suffix   = std::move(suffix);
        return f;
    }
    [[nodiscard]] static ValueFormat integer(int pad = 0)
    {
        ValueFormat f;
        f.decimals = 0;
        f.pad      = pad;
        return f;
    }
    [[nodiscard]] static ValueFormat duration(int units = 2)
    {
        ValueFormat f;
        f.kind     = Kind::Duration;
        f.decimals = units;
        return f;
    }
    [[nodiscard]] static ValueFormat percent(int decimals = 1)
    {
        ValueFormat f;
        f.kind     = Kind::Percent;
        f.decimals = decimals;
        return f;
    }
};

/// Decimals that show a tick spacing of `step` exactly (0.25 -> 2, 5 -> 0).
[[nodiscard]] int decimalsFor(double step);

} // namespace sml
