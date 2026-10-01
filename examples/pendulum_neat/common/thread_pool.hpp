#pragma once
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace tp
{

/// Fixed workers splitting a range. Same `dispatch` contract as the original pool, but idle workers
/// sleep on a condition variable instead of spinning.
class ThreadPool
{
public:
    explicit ThreadPool(uint32_t thread_count)
    {
        for (uint32_t i{0}; i < thread_count; ++i) {
            m_workers.emplace_back([this, i] { run(i); });
        }
    }

    ~ThreadPool()
    {
        {
            std::lock_guard lock{m_mutex};
            m_stop = true;
        }
        m_wake.notify_all();
        for (auto& w : m_workers) {
            w.join();
        }
    }

    ThreadPool(ThreadPool const&)            = delete;
    ThreadPool& operator=(ThreadPool const&) = delete;

    [[nodiscard]] uint32_t size() const { return static_cast<uint32_t>(m_workers.size()); }

    /// Calls `callback(start, end)` on disjoint chunks covering [0, element_count) and waits.
    template<typename TCallback>
    void dispatch(uint32_t element_count, TCallback&& callback)
    {
        uint32_t const n     = size();
        uint32_t const batch = n ? element_count / n : 0;
        {
            std::lock_guard lock{m_mutex};
            m_job     = [&](uint32_t i) { callback(batch * i, batch * (i + 1)); };
            m_pending = n;
            ++m_generation;
        }
        m_wake.notify_all();
        if (batch * n < element_count) {
            callback(batch * n, element_count);
        }
        std::unique_lock lock{m_mutex};
        m_done.wait(lock, [this] { return m_pending == 0; });
        m_job = nullptr;
    }

private:
    void run(uint32_t index)
    {
        uint64_t seen = 0;
        while (true) {
            std::function<void(uint32_t)> job;
            {
                std::unique_lock lock{m_mutex};
                m_wake.wait(lock, [&] { return m_stop || m_generation != seen; });
                if (m_stop) {
                    return;
                }
                seen = m_generation;
                job  = m_job;
            }
            job(index);
            {
                std::lock_guard lock{m_mutex};
                --m_pending;
            }
            m_done.notify_one();
        }
    }

    std::vector<std::thread>      m_workers;
    std::mutex                    m_mutex;
    std::condition_variable       m_wake, m_done;
    std::function<void(uint32_t)> m_job;
    uint32_t                      m_pending    = 0;
    uint64_t                      m_generation = 0;
    bool                          m_stop       = false;
};

} // namespace tp
