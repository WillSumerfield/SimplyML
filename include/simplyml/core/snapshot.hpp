#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <utility>

namespace sml
{

/// Latest value of some state (e.g. training stats), written from any thread and read by the UI.
/// `get()` is for a single reader thread: it copies the value only when it changed, so per-frame
/// reads cost one atomic load.
template<typename T>
class Snapshot
{
public:
    Snapshot() = default;
    explicit Snapshot(T value)
        : m_back{value}
        , m_front{std::move(value)}
    {}

    void set(T value)
    {
        std::lock_guard lock{m_mutex};
        m_back = std::move(value);
        m_version.fetch_add(1, std::memory_order_release);
    }

    /// Edits the value in place under the lock.
    template<typename F>
    void update(F&& f)
    {
        std::lock_guard lock{m_mutex};
        std::forward<F>(f)(m_back);
        m_version.fetch_add(1, std::memory_order_release);
    }

    /// Reader-thread view; valid until the next `get()`.
    T const& get()
    {
        if (m_version.load(std::memory_order_acquire) != m_readVersion) {
            std::lock_guard lock{m_mutex};
            m_front       = m_back;
            m_readVersion = m_version.load(std::memory_order_relaxed);
        }
        return m_front;
    }

    /// Copy of the latest value, from any thread.
    [[nodiscard]] T load() const
    {
        std::lock_guard lock{m_mutex};
        return m_back;
    }

    /// Bumps on every write; compare to detect changes.
    [[nodiscard]] std::uint64_t version() const { return m_version.load(std::memory_order_acquire); }

private:
    mutable std::mutex         m_mutex;
    T                          m_back{};
    T                          m_front{};
    std::atomic<std::uint64_t> m_version{0};
    std::uint64_t              m_readVersion = 0;
};

} // namespace sml
