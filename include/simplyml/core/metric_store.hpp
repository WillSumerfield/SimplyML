#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace sml
{

struct MetricPoint
{
    double step  = 0.0;
    double value = 0.0;
};

/// One named metric's history, as seen by the UI thread.
class Series
{
public:
    [[nodiscard]] std::string const&             name() const   { return m_name; }
    [[nodiscard]] std::deque<MetricPoint> const& points() const { return m_points; }
    [[nodiscard]] std::size_t                    size() const   { return m_points.size(); }
    [[nodiscard]] bool                           empty() const  { return m_points.empty(); }
    [[nodiscard]] MetricPoint const&             last() const   { return m_points.back(); }
    /// Points ever appended (including dropped ones); `total() - size()` is the index of `points()[0]`.
    [[nodiscard]] std::uint64_t                  total() const  { return m_total; }
    /// 0 = unlimited.
    [[nodiscard]] std::size_t                    maxPoints() const { return m_maxPoints; }

private:
    friend class MetricStore;

    std::string              m_name;
    std::deque<MetricPoint>  m_points;
    std::vector<MetricPoint> m_inbox; // swapped with the writer buffer on sync
    std::uint64_t            m_total     = 0;
    std::size_t              m_maxPoints = 0;
};

/// Named scalar series fed by training code and read by the UI.
///
/// Writers (`push*`, `setMaxPoints`, `clear`) are thread-safe and only append to a pending buffer
/// under a short lock. The UI thread calls `sync()` once per frame to move pending points into the
/// series it reads (`series`, `names`). Dumps (`writeCsv`, `writeBinary`) are safe from any thread.
class MetricStore
{
public:
    void push(std::string_view name, double step, double value);
    /// Step = previous step of this series + 1 (0 for the first point).
    void push(std::string_view name, double value);
    void pushMany(std::string_view name, double const* steps, double const* values, std::size_t count);
    /// Keeps only the newest `maxPoints` (0 = unlimited, the default). Applied on the next sync.
    void setMaxPoints(std::string_view name, std::size_t maxPoints);
    /// Drops all series and pending points; applied on the next sync.
    void clear();

    /// UI thread: applies pending writes. Returns true if anything changed.
    bool sync();

    /// UI thread: nullptr if `name` has never been synced.
    [[nodiscard]] Series const*            series(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> names() const;

    /// Long format: `series,step,value`, one row per point.
    void writeCsv(std::filesystem::path const& path) const;
    void writeBinary(std::filesystem::path const& path) const;
    /// Replaces the synced series with a `writeBinary` dump (UI thread, or before the App starts).
    void readBinary(std::filesystem::path const& path);

private:
    struct Pending
    {
        std::vector<MetricPoint> points;
        double                   lastStep  = -1.0;
        std::size_t              maxPoints = 0;
        bool                     maxSet    = false;
    };

    Pending& pending(std::string_view name); // m_writeMutex held

    std::mutex                                   m_writeMutex;
    std::map<std::string, Pending, std::less<>>  m_pending;
    bool                                         m_clearRequested = false;

    mutable std::mutex                          m_readMutex; // sync vs dumps from other threads
    std::map<std::string, Series, std::less<>>  m_series;
};

} // namespace sml
