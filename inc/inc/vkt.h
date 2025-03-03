#pragma once
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <volk.h>

template <typename T>
class ThreadSafeQueue
{
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;

public:
    void push(T value)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(value));
        }
        m_cond.notify_one();
    }

    bool tryPop(T &value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
        {
            return false;
        }
        value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    void waitPop(T &value)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this] { return !m_queue.empty(); });
        value = std::move(m_queue.front());
        m_queue.pop();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
};

