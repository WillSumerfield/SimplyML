#include "simplyml/core/metric_store.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace sml
{

namespace
{

constexpr char          kMagic[4] = {'S', 'M', 'L', 'M'};
constexpr std::uint32_t kVersion  = 1;

template<typename T>
void writePod(std::ostream& out, T const& v)
{
    out.write(reinterpret_cast<char const*>(&v), sizeof(T));
}

template<typename T>
T readPod(std::istream& in)
{
    T v{};
    in.read(reinterpret_cast<char*>(&v), sizeof(T));
    return v;
}

void trim(std::deque<MetricPoint>& points, std::size_t maxPoints)
{
    if (maxPoints && points.size() > maxPoints) {
        points.erase(points.begin(), points.end() - static_cast<std::ptrdiff_t>(maxPoints));
    }
}

} // namespace

MetricStore::Pending& MetricStore::pending(std::string_view name)
{
    auto it = m_pending.find(name);
    if (it == m_pending.end()) {
        it = m_pending.emplace(std::string(name), Pending{}).first;
    }
    return it->second;
}

void MetricStore::push(std::string_view name, double step, double value)
{
    std::lock_guard lock{m_writeMutex};
    Pending& p = pending(name);
    p.points.push_back({step, value});
    p.lastStep = step;
}

void MetricStore::push(std::string_view name, double value)
{
    std::lock_guard lock{m_writeMutex};
    Pending& p = pending(name);
    p.lastStep += 1.0;
    p.points.push_back({p.lastStep, value});
}

void MetricStore::pushMany(std::string_view name, double const* steps, double const* values, std::size_t count)
{
    if (count == 0) {
        return;
    }
    std::lock_guard lock{m_writeMutex};
    Pending& p = pending(name);
    p.points.reserve(p.points.size() + count);
    for (std::size_t i = 0; i < count; ++i) {
        p.points.push_back({steps[i], values[i]});
    }
    p.lastStep = steps[count - 1];
}

void MetricStore::setMaxPoints(std::string_view name, std::size_t maxPoints)
{
    std::lock_guard lock{m_writeMutex};
    Pending& p = pending(name);
    p.maxPoints = maxPoints;
    p.maxSet    = true;
}

void MetricStore::clear()
{
    std::lock_guard lock{m_writeMutex};
    m_pending.clear();
    m_clearRequested = true;
}

bool MetricStore::sync()
{
    std::lock_guard readLock{m_readMutex};
    bool changed = false;
    {
        std::lock_guard lock{m_writeMutex};
        if (m_clearRequested) {
            m_series.clear();
            m_clearRequested = false;
            changed          = true;
        }
        for (auto& [name, p] : m_pending) {
            if (p.points.empty() && !p.maxSet) {
                continue;
            }
            auto it = m_series.find(name);
            if (it == m_series.end()) {
                it = m_series.emplace(name, Series{}).first;
                it->second.m_name = name;
            }
            Series& s = it->second;
            s.m_inbox.swap(p.points); // p.points keeps the inbox's (empty) capacity
            if (p.maxSet) {
                s.m_maxPoints = p.maxPoints;
                p.maxSet      = false;
            }
        }
    }
    // append outside the writer lock
    for (auto& [name, s] : m_series) {
        if (!s.m_inbox.empty()) {
            s.m_points.insert(s.m_points.end(), s.m_inbox.begin(), s.m_inbox.end());
            s.m_total += s.m_inbox.size();
            s.m_inbox.clear();
            changed = true;
        }
        trim(s.m_points, s.m_maxPoints);
    }
    return changed;
}

Series const* MetricStore::series(std::string_view name) const
{
    auto const it = m_series.find(name);
    return it == m_series.end() ? nullptr : &it->second;
}

std::vector<std::string> MetricStore::names() const
{
    std::vector<std::string> out;
    out.reserve(m_series.size());
    for (auto const& [name, s] : m_series) {
        out.push_back(name);
    }
    return out;
}

void MetricStore::writeCsv(std::filesystem::path const& path) const
{
    std::ofstream out{path};
    if (!out) {
        throw std::runtime_error("SimplyML: cannot write '" + path.string() + "'");
    }
    out.precision(17);
    out << "series,step,value\n";
    std::lock_guard lock{m_readMutex};
    for (auto const& [name, s] : m_series) {
        for (auto const& p : s.m_points) {
            out << name << ',' << p.step << ',' << p.value << '\n';
        }
    }
}

void MetricStore::writeBinary(std::filesystem::path const& path) const
{
    std::ofstream out{path, std::ios::binary};
    if (!out) {
        throw std::runtime_error("SimplyML: cannot write '" + path.string() + "'");
    }
    std::lock_guard lock{m_readMutex};
    out.write(kMagic, sizeof(kMagic));
    writePod(out, kVersion);
    writePod(out, static_cast<std::uint32_t>(m_series.size()));
    for (auto const& [name, s] : m_series) {
        writePod(out, static_cast<std::uint32_t>(name.size()));
        out.write(name.data(), static_cast<std::streamsize>(name.size()));
        writePod(out, static_cast<std::uint64_t>(s.m_points.size()));
        for (auto const& p : s.m_points) {
            writePod(out, p.step);
            writePod(out, p.value);
        }
    }
}

void MetricStore::readBinary(std::filesystem::path const& path)
{
    std::ifstream in{path, std::ios::binary};
    char magic[4] = {};
    in.read(magic, sizeof(magic));
    if (!in || std::memcmp(magic, kMagic, sizeof(kMagic)) != 0 || readPod<std::uint32_t>(in) != kVersion) {
        throw std::runtime_error("SimplyML: '" + path.string() + "' is not a SimplyML metric dump");
    }
    std::map<std::string, Series, std::less<>> loaded;
    auto const count = readPod<std::uint32_t>(in);
    for (std::uint32_t i = 0; i < count && in; ++i) {
        std::string name(readPod<std::uint32_t>(in), '\0');
        in.read(name.data(), static_cast<std::streamsize>(name.size()));
        Series& s = loaded[name];
        s.m_name  = name;
        auto const n = readPod<std::uint64_t>(in);
        for (std::uint64_t j = 0; j < n && in; ++j) {
            MetricPoint p;
            p.step  = readPod<double>(in);
            p.value = readPod<double>(in);
            s.m_points.push_back(p);
        }
        s.m_total = s.m_points.size();
    }
    if (!in) {
        throw std::runtime_error("SimplyML: truncated metric dump '" + path.string() + "'");
    }
    std::lock_guard lock{m_readMutex};
    m_series = std::move(loaded);
}

} // namespace sml
