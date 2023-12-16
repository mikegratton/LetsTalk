#pragma once
#include <chrono>
#include <condition_variable>
#include <initializer_list>
#include <mutex>
#include <vector>

#include "Awaitable.hpp"

namespace lt {

/**
 * @brief Wait for any attached Awaitable to have messages.
 *
 * This is an efficient way of having one consumer thread wait
 * on multiple producers.
 */
class Waitset {
   public:
    /**
     * @brief Make a waitset
     */
    Waitset(std::initializer_list<AwaitablePtr> i_watched) : m_watched(i_watched)
    {
        for (auto& watch : m_watched) { watch->attachToCondition(&m_waitset); }
    }

    /// Detach from all queues
    ~Waitset()
    {
        for (auto& watch : m_watched) { watch->detachFromCondition(); }
    }

    /// Add an awaitable to watch
    int attach(AwaitablePtr i_watch)
    {
        i_watch->attachToCondition(&m_waitset);
        m_watched.push_back(i_watch);
        return static_cast<int>(m_watched.size() - 1);
    }

    /// Retrieve the type-erased pointer
    AwaitablePtr get(int i)
    {
        if (i >= 0 && i < m_watched.size()) { return m_watched[i]; }
        return nullptr;
    }

    /**
     * @brief Wait for messages in one of the queues
     * @param i_timeout maximum wait duration
     * @return Index in tuple of queue with data or -1 on timeout
     *
     * Waits for one of the attached queues to have data. Note that if
     * there are multiple consumers, the wakeup may be spurious -- none of the queues may
     * actually have data. The returned value is always the lowest
     * numbered queue with data. Higher numbered queues may also
     * have data.
     */
    int wait(std::chrono::milliseconds i_timeout = std::chrono::hours(72))
    {
        // Check for mail at entry
        for (std::size_t i = 0; i < m_watched.size(); i++) {
            if (m_watched[i]->ready()) { return i; }
        }

        // Check repeatedly afterwards
        std::unique_lock<std::mutex> guard(m_mutex);
        for (;;) {
            auto status = m_waitset.wait_for(guard, i_timeout);
            if (status == std::cv_status::timeout) { return -1; }
            for (std::size_t i = 0; i < m_watched.size(); i++) {
                if (m_watched[i]->ready()) { return i; }
            }
        }
    }

    /**
     * @brief Wait for messages in one of the queues
     * @param i_waitUntil After this time, wait will return regardless
     * @return Index in tuple of queue with data or -1 on timeout
     *
     * Waits for one of the attached queues to have data. Note that if
     * there are multiple consumers, the wakeup may be spurious -- none of the queues may
     * actually have data. The returned value is always the lowest
     * numbered queue with data. Higher numbered queues may also
     * have data.
     */
    int wait(std::chrono::steady_clock::time_point i_waitUntil)
    {
        // Check for mail at entry
        for (std::size_t i = 0; i < m_watched.size(); i++) {
            if (m_watched[i]->ready()) { return i; }
        }

        // Check repeatedly afterwards
        std::unique_lock<std::mutex> guard(m_mutex);
        for (;;) {
            auto status = m_waitset.wait_until(guard, i_waitUntil);
            if (status == std::cv_status::timeout) { return -1; }
            for (std::size_t i = 0; i < m_watched.size(); i++) {
                if (m_watched[i]->ready()) { return i; }
            }
        }
    }

   protected:
    std::vector<AwaitablePtr> m_watched;
    mutable std::mutex m_mutex;
    std::condition_variable m_waitset;
};

}  // namespace lt