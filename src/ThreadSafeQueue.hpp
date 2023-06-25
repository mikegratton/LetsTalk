#pragma once
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

#include "Awaitable.hpp"

namespace lt {

/**
 * @brief A simple producer/subscriber queue using unique_ptr
 *
 * A thread-safe queue storing a deque of unique_ptr<T>'s. Supports bulk push/pop
 * operations. This queue is optionally bounded. If more items that the high
 * water mark are inserted, the oldest items are discarded to keep the queue to the
 * desired maximum size
 */
template <class T>
class ThreadSafeQueue : public Awaitable {
   public:
    using Queue = std::deque<std::unique_ptr<T>>;

    /// Construct a queue with an optional capacity bound
    /// @param i_capacity If nonzero, discard old samples when the queue length exceeds this value
    ThreadSafeQueue(std::size_t i_capacity = 0) : m_capacity(i_capacity), m_externalCondition(nullptr) {}

    /// Number of samples in the queue
    std::size_t size() const
    {
        LockGuard guard(m_mutex);
        return m_queue.size();
    }

    /// Max capacity of the queue. If zero, the length is unlimited
    std::size_t capacity() const { return m_capacity; }

    /// Check if the queue is empty
    bool empty() const
    {
        LockGuard guard(m_mutex);
        return m_queue.empty();
    }

    /// Empty the queue
    void clear()
    {
        LockGuard guard(m_mutex);
        m_queue.clear();
    }

    /**
     * @brief Return the front element of the queue, removing it from the queue.
     *
     * If the queue is empty or if it is locked for longer than i_wait,
     * return nullptr.
     *
     * @param i_wait Wait duration for access and data
     */
    std::unique_ptr<T> pop(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0))
    {
        LockGuard guard(m_mutex);
        if (m_nonempty.wait_for(guard, i_wait, [this]() { return !m_queue.empty(); })) {
            auto popped = std::move(m_queue.front());
            m_queue.pop_front();
            return popped;
        }
        return nullptr;
    }

    /**
     * @brief Return the contents queue, emptying it.
     *
     * If the queue is empty or if it is locked for longer than i_wait, return an empty list.
     *
     * @param i_wait Wait duration for access and data
     */
    Queue popAll(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0))
    {
        Queue queue;
        LockGuard guard(m_mutex);
        if (m_nonempty.wait_for(guard, i_wait, [this]() { return !m_queue.empty(); })) { queue.swap(m_queue); }
        return queue;
    }

    /**
     * @brief Move data onto back of queue
     */
    void push(std::unique_ptr<T> i_data)
    {
        LockGuard guard(m_mutex);
        m_queue.push_back(std::move(i_data));
        if (m_capacity > 0 && m_queue.size() > m_capacity) { m_queue.pop_front(); }
        guard.unlock();
        m_nonempty.notify_one();
        if (m_externalCondition) { m_externalCondition->notify_one(); }
    }

    /**
     * @brief Copy data onto the back of the queue
     */
    void push(T const& i_data)
    {
        std::unique_ptr<T> dataPtr(new T);
        *dataPtr = i_data;
        push(std::move(dataPtr));
    }

    /**
     * @brief Move all data onto back of queue
     */
    void pushAll(Queue&& i_data)
    {
        LockGuard guard(m_mutex);
        for (auto& item : i_data) { m_queue.emplace_back(std::move(item)); }
        while (m_capacity > 0 && m_queue.size() > m_capacity) { m_queue.pop_front(); }
        guard.unlock();
        i_data.clear();
        m_nonempty.notify_one();
        if (m_externalCondition) { m_externalCondition->notify_one(); }
    }

    /**
     * @brief Construct T onto the back of the queue
     */
    template <class... Args>
    void emplace(Args&&... i_args)
    {
        LockGuard guard(m_mutex);
        m_queue.emplace_back(new T(std::forward<Args>(i_args)...));
        while (m_capacity > 0 && m_queue.size() > m_capacity) { m_queue.pop_front(); }
        guard.unlock();
        m_nonempty.notify_one();
        if (m_externalCondition) { m_externalCondition->notify_one(); }
    }

    /**
     * @brief Swap contents with another queue. Handles deadlock avoidance.
     */
    void swap(ThreadSafeQueue& io_other)
    {
        LockGuard guard1(m_mutex, std::defer_lock);
        LockGuard guard2(io_other.m_mutex, std::defer_lock);
        std::lock(guard1, guard2);
        m_queue.swap(io_other.m_queue);
    }

    /**
     * @brief Attach to another condition variable
     * @note Only one external condition may be attached at a time. (A queue can only be in one waitset)
     */
    void attachToCondition(std::condition_variable* i_condition) final { m_externalCondition = i_condition; }

    /// Detach the external condition variable
    void detachFromCondition() final { m_externalCondition = nullptr; }

    /// Check if data is available
    bool ready() const final { return !empty(); }

   protected:
    const std::size_t m_capacity;

    Queue m_queue;
    mutable std::mutex m_mutex;
    using LockGuard = std::unique_lock<std::mutex>;
    std::condition_variable m_nonempty;
    std::condition_variable* m_externalCondition;
};

template <class T>
using QueuePtr = std::shared_ptr<ThreadSafeQueue<T>>;

}  // namespace lt
