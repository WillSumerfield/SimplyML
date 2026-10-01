#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

namespace sml
{

/// Fixed-capacity history; pushing when full drops the oldest value.
/// Index 0 is the oldest value, `size() - 1` the newest. Keeps a running sum for O(1) `mean()`;
/// the sum is recomputed on every wrap so float drift stays bounded.
template<typename T>
class RingBuffer
{
public:
    explicit RingBuffer(std::size_t capacity = 8)
        : m_data(std::max<std::size_t>(capacity, 1))
    {}

    void push(T const& v)
    {
        if (m_size == m_data.size()) {
            m_sum = m_sum - m_data[m_head];
        } else {
            ++m_size;
        }
        m_data[m_head] = v;
        m_sum          = m_sum + v;
        if (++m_head == m_data.size()) {
            m_head = 0;
            recomputeSum();
        }
    }

    void clear()
    {
        m_head = 0;
        m_size = 0;
        m_sum  = T{};
    }

    /// Changes capacity, keeping the newest values that fit.
    void setCapacity(std::size_t capacity)
    {
        capacity = std::max<std::size_t>(capacity, 1);
        std::vector<T> data(capacity);
        std::size_t const keep = std::min(m_size, capacity);
        for (std::size_t i = 0; i < keep; ++i) {
            data[i] = (*this)[m_size - keep + i];
        }
        m_data = std::move(data);
        m_size = keep;
        m_head = keep % capacity;
        recomputeSum();
    }

    [[nodiscard]] std::size_t size() const     { return m_size; }
    [[nodiscard]] std::size_t capacity() const { return m_data.size(); }
    [[nodiscard]] bool        empty() const    { return m_size == 0; }
    [[nodiscard]] bool        full() const     { return m_size == m_data.size(); }

    /// i = 0 is the oldest value.
    [[nodiscard]] T const& operator[](std::size_t i) const
    {
        assert(i < m_size);
        std::size_t const start = m_size == m_data.size() ? m_head : 0;
        std::size_t j = start + i;
        if (j >= m_data.size()) {
            j -= m_data.size();
        }
        return m_data[j];
    }

    [[nodiscard]] T const& front() const { return (*this)[0]; }
    [[nodiscard]] T const& back() const  { return (*this)[m_size - 1]; }

    [[nodiscard]] T sum() const { return m_sum; }

    /// Mean of stored values; `T{}` when empty.
    [[nodiscard]] T mean() const
    {
        return m_size ? m_sum / static_cast<float>(m_size) : T{};
    }

    /// Newest minus oldest; `T{}` when empty.
    [[nodiscard]] T diff() const
    {
        return m_size ? back() - front() : T{};
    }

    /// Calls `cb(i, value)` from oldest to newest.
    template<typename TCallback>
    void forEach(TCallback&& cb) const
    {
        for (std::size_t i = 0; i < m_size; ++i) {
            cb(i, (*this)[i]);
        }
    }

private:
    void recomputeSum()
    {
        m_sum = T{};
        for (std::size_t i = 0; i < m_size; ++i) {
            m_sum = m_sum + (*this)[i];
        }
    }

    std::vector<T> m_data;
    std::size_t    m_head = 0; // next write slot
    std::size_t    m_size = 0;
    T              m_sum{};
};

} // namespace sml
