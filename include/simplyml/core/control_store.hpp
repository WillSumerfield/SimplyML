#pragma once
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sml
{

/// Named control values (slider positions, toggle states, ...), shared between the widgets that
/// edit them and training code that reads them. Every call is thread-safe and short.
///
/// Each write gets a new version, so a reader can tell whether a value changed since it last looked.
class ControlStore
{
public:
    struct Entry
    {
        double        value   = 0.0;
        std::uint64_t version = 0;
    };

    /// Writes `value` and returns its new version.
    std::uint64_t set(std::string_view name, double value);
    /// Creates `name` with `value` unless it exists; returns the current value.
    double declare(std::string_view name, double value);

    [[nodiscard]] std::optional<double> get(std::string_view name) const;
    [[nodiscard]] double                get(std::string_view name, double fallback) const;
    [[nodiscard]] std::optional<Entry>  entry(std::string_view name) const;
    [[nodiscard]] bool                  contains(std::string_view name) const;
    /// Every (name, value), sorted by name.
    [[nodiscard]] std::vector<std::pair<std::string, double>> all() const;

private:
    mutable std::mutex                         m_mutex;
    std::map<std::string, Entry, std::less<>> m_values;
    std::uint64_t                              m_version = 0;
};

} // namespace sml
