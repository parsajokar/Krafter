#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

namespace Krafter {

template <typename T>
class ResultQueue {
public:
    void Push(T value)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Items.push_back(std::move(value));
    }

    std::deque<T> Drain()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::deque<T> out;
        out.swap(m_Items);
        return out;
    }

    std::deque<T> DrainUpTo(size_t max)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::deque<T> out;
        size_t take = std::min(max, m_Items.size());
        for (size_t i = 0; i < take; i++) {
            out.push_back(std::move(m_Items.front()));
            m_Items.pop_front();
        }
        return out;
    }

private:
    std::mutex m_Mutex;
    std::deque<T> m_Items;
};

}
