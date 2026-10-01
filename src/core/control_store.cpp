#include "simplyml/core/control_store.hpp"

namespace sml
{

std::uint64_t ControlStore::set(std::string_view name, double value)
{
    std::lock_guard lock{m_mutex};
    auto it = m_values.find(name);
    if (it == m_values.end()) {
        it = m_values.emplace(std::string(name), Entry{}).first;
    }
    it->second = {value, ++m_version};
    return m_version;
}

double ControlStore::declare(std::string_view name, double value)
{
    std::lock_guard lock{m_mutex};
    if (auto it = m_values.find(name); it != m_values.end()) {
        return it->second.value;
    }
    m_values.emplace(std::string(name), Entry{value, ++m_version});
    return value;
}

std::optional<double> ControlStore::get(std::string_view name) const
{
    std::lock_guard lock{m_mutex};
    auto it = m_values.find(name);
    return it == m_values.end() ? std::nullopt : std::optional{it->second.value};
}

double ControlStore::get(std::string_view name, double fallback) const
{
    return get(name).value_or(fallback);
}

std::optional<ControlStore::Entry> ControlStore::entry(std::string_view name) const
{
    std::lock_guard lock{m_mutex};
    auto it = m_values.find(name);
    return it == m_values.end() ? std::nullopt : std::optional{it->second};
}

bool ControlStore::contains(std::string_view name) const
{
    std::lock_guard lock{m_mutex};
    return m_values.find(name) != m_values.end();
}

std::vector<std::pair<std::string, double>> ControlStore::all() const
{
    std::lock_guard lock{m_mutex};
    std::vector<std::pair<std::string, double>> out;
    out.reserve(m_values.size());
    for (auto const& [k, e] : m_values) {
        out.emplace_back(k, e.value);
    }
    return out;
}

} // namespace sml
