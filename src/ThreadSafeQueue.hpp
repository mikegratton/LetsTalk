#pragma once
#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace lt {

/**
 * A thread-safe queue storing a list of unique_ptr<T>'s. Supports bulk push/pop
 * operations. This queue is optionally bounded. If more items that the high
 * water mark are inserted, the oldest items are discarded to keep the queue to the
 * desired maximum size
 */
template<class T>
class ThreadSafeQueue {
public:

    using Queue = std::list<std::unique_ptr<T>>;

    ThreadSafeQueue(std::size_t i_capacity = 0)
        : m_capacity(i_capacity) { }

    std::size_t size() const { LockGuard guard(m_mutex); return m_queue.size(); }

    std::size_t capacity() const { return m_capacity; }
    
    bool empty() const { LockGuard guard(m_mutex); return m_queue.empty(); }

    void clear() { LockGuard guard(m_mutex); m_queue.clear(); }
    

    /**
     * Return the front element of the queue, removing it from the queue.
     * If the queue is empty or if it is locked for longer than i_wait,
     * return nullptr.
     *
     * @param i_wait Wait duration for access and data
     */
    std::unique_ptr<T> pop(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0)) {
        LockGuard guard(m_mutex);
        if (m_nonempty.wait_for(guard, i_wait, [this]() { return !m_queue.empty(); })) {
            auto popped = std::move(m_queue.front());
            m_queue.pop_front();
            return popped;
        }
        return nullptr;
    }

    /**
     * Return the contents queue, emptying it. If the queue is empty or if
     * it is locked for longer than i_wait, return an empty list.
     *
     * @param i_wait Wait duration for access and data
     */
    Queue popAll(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0)) {
        Queue queue;
        LockGuard guard(m_mutex);
        if (m_nonempty.wait_for(guard, i_wait, [this]() { return !m_queue.empty(); })) {
            queue.swap(m_queue);
            return queue;
        }
        return queue;
    }

    /**
     * Move data onto back of queue
     */
    void push(std::unique_ptr<T> i_data) {
        LockGuard guard(m_mutex);
        m_queue.push_back(std::move(i_data));
        if (m_capacity > 0 && m_queue.size() > m_capacity) {
            m_queue.pop_front();
        }
        guard.unlock();
        m_nonempty.notify_one();
    }

    /**
     * Move all data onto back of queue
     */
    void pushAll(Queue&& i_data) {
        LockGuard guard(m_mutex);
        m_queue.splice(m_queue.end(), i_data);
        while (m_capacity > 0 && m_queue.size() > m_capacity) {
            m_queue.pop_front();
        }
        guard.unlock();
        m_nonempty.notify_one();
    }

    /**
     * Construct T onto the back of the queue
     */
    template<class... Args>
    void emplace(Args&&... i_args) {
        LockGuard guard(m_mutex);
        m_queue.emplace_back(new T(std::forward<Args>(i_args)...));
        while (m_capacity > 0 && m_queue.size() > m_capacity) {
            m_queue.pop_front();
        }
        guard.unlock();
        m_nonempty.notify_one();
    }

    /*
     * Swap contents with another queue. Handle deadlock avoidance.
     */
    void swap(ThreadSafeQueue& io_other) {
        LockGuard guard1(m_mutex, std::defer_lock);
        LockGuard guard2(io_other.m_mutex, std::defer_lock);
        std::lock(guard1, guard2);
        m_queue.swap(io_other.m_queue);
    }

protected:
    const std::size_t m_capacity;

    Queue m_queue;
    mutable std::mutex m_mutex;
    using LockGuard = std::unique_lock<std::mutex>;
    std::condition_variable m_nonempty;
};

template<class T>
using QueuePtr = std::shared_ptr<ThreadSafeQueue<T>>;

}
