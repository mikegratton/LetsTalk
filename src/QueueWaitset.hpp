#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>

#include "ThreadSafeQueue.hpp"
#include "meta.hpp"

namespace lt {

/**
 * @brief Wait for any attached ThreadSafeQueue to have messages.
 *
 * This is an efficient way of having one consumer thread wait
 * on multiple producers coming from subscriptions.
 */
template <class... Ts>
class QueueWaitset {
   public:
    using QueueTuple = std::tuple<QueuePtr<Ts>...>;

    /**
     *
     *  @brief See makeQueueWaitset() for a convenient builder
     *
     *  Note that the call should look like
     *
     * ```cpp
     *  auto node = lt::Participant::create();
     *  auto type1Sub = node->subscribe<MyType1>("topic1");
     *  auto type2Sub = node->subscribe<MyType2>("topic2");
     *  QueueWaitset<MyType1, MyType2> waitset({type1Sub, type2Sub});
     * ```
     *
     */
    QueueWaitset(QueueTuple const& i_queue) : m_queue(i_queue)
    {
        AttachFunctor attach(&m_waitset);
        for_each_in_tuple(m_queue, attach);
    }

    /**
     * @brief Wait for messages in one of the queues
     *
     * @return Index in tuple of queue with data
     *
     * Waits (potentially forever) for one of the attached queues
     * to have data. Note that if there are multiple consumers,
     * the wakeup may be spurious -- none of the queues may
     * actually have data. The returned value is always the lowest
     * numbered queue with data. Higher numbered queues may also
     * have data.
     */
    int wait()
    {
        CheckMailFunctor check;
        std::unique_lock<std::mutex> guard(m_mutex);
        for (;;) {
            m_waitset.wait(guard);
            for_each_in_tuple(m_queue, check);
            if (check.m_pending >= 0) { return check.m_pending; }
        }
    }

   protected:
    // Curry condition variable for attaching to all queues
    struct AttachFunctor {
        std::condition_variable* m_condition;

        AttachFunctor(std::condition_variable* i_condition) : m_condition(i_condition) {}

        template <class T>
        void operator()(T& io_queue)
        {
            io_queue->attachWaitsetCondition(m_condition);
        }
    };

    // Curry communication ints
    struct CheckMailFunctor {
        int m_sequence;  // Tracks which queue in tuple we are considering
        int m_pending;   // Holds lowest indexed queue with data

        CheckMailFunctor() : m_sequence(0), m_pending(-1) {}

        template <class T>
        void operator()(T const& io_queue)
        {
            int sequenceIndex = m_sequence++;
            if (m_pending >= 0) { return; }
            if (!io_queue->empty()) { m_pending = sequenceIndex; }
        }
    };

    QueueTuple m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_waitset;
};

/**
 * @brief Build a waitset out of one or more QueuePtr<T>'s
 *
 * Example:
 * ```cpp
 *  auto node = lt::Participant::create();
 *  auto type1Sub = node->subscribe<MyType1>("topic1");
 *  auto type2Sub = node->subscribe<MyType2>("topic2");
 *  auto waitset = makeQueueWaitset(type1Sub, type2Sub);
 * ```
 */
template <class... Ts>
auto makeQueueWaitset(QueuePtr<Ts>&... io_waitsets)
{
    return QueueWaitset<Ts...>(std::make_tuple(io_waitsets...));
}

}  // namespace lt