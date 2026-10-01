#include "simplyml/ui/value_format.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "simplyml/util/format.hpp"

namespace sml
{

std::string ValueFormat::operator()(double v) const
{
    if (custom) {
        return custom(v);
    }
    std::string body;
    switch (kind) {
        case Kind::Duration:
            body = formatDuration(v, std::max(decimals, 1));
            break;
        case Kind::Percent:
            body = toString(v * 100.0, decimals) + "%";
            break;
        case Kind::Scientific:
        case Kind::General: {
            char buf[64];
            std::snprintf(buf, sizeof(buf), kind == Kind::Scientific ? "%.*e" : "%.*g",
                          std::max(decimals, kind == Kind::General ? 1 : 0), v);
            body = buf;
            break;
        }
        case Kind::Number:
            if (!std::isfinite(v)) {
                body = std::isnan(v) ? "nan" : (v > 0 ? "inf" : "-inf");
                break;
            }
            body = toString(v, std::max(decimals, 0));
            if (pad > 0) {
                bool const neg   = !body.empty() && body[0] == '-';
                auto const dot   = body.find('.');
                auto const digits = static_cast<int>((dot == std::string::npos ? body.size() : dot) - (neg ? 1 : 0));
                if (digits < pad) {
                    body.insert(neg ? 1 : 0, static_cast<std::size_t>(pad - digits), '0');
                }
            }
            break;
    }
    return prefix + body + suffix;
}

int decimalsFor(double step)
{
    if (!(step > 0.0) || !std::isfinite(step)) {
        return 0;
    }
    // Round away float noise (0.1 * 3 = 0.30000000000000004) before counting digits.
    for (int d = 0; d < 12; ++d) {
        double const scaled = step * std::pow(10.0, d);
        if (std::abs(scaled - std::round(scaled)) < 1e-6 * std::max(1.0, scaled)) {
            return d;
        }
    }
    return 12;
}

} // namespace sml
