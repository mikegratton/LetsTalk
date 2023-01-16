#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <list>

namespace lt 
{
    
/**
 * ActiveObject is a virtual base class for the active object pattern.
 * This creates a private thread with an internal queue of jobs. The jobs
 * are run one at a time in order of insertion into the queue.
 * 
 * A typical pattern looks like
 * ```
 * class Counter : public ActiveObject
 * {
 * public:
 * 
 * Counter() : m_count(0) { }
 * 
 * void add(int i_amount) 
 * {
 *    submitJob([i_amount, this]() { 
 *      m_count += i_amount; 
 *    });
 * }
 * 
 * std::future<int> value() 
 * {
 *    // Note: we must handle the promise via a pointer to caputre it correctly.
 *    auto promise = std::make_shared<std::promise<int>>();
 *    submitJob([promise, this]() {
 *      promise->set_value(m_count);
 *    });   
 *    return promise->get_future();
 * }
 * 
 * protected:
 * 
 *   int m_count;   
 * 
 * };
 * ```
 * 
 * Notice that no explicit mutex is required to protect m_count.  Calls to add()
 * and value() from different threads will occur in race order, but only one will be
 * executed at a time.
 * 
 * By convention, the private members of an active object run on the private thread.
 * That is, they are used by lambdas defined inside of submitJob(). This allows the object
 * to be programmed as if it were not thread safe for its interior business logic. Return
 * values must all be done via std::future.
 * 
 */
    
class ActiveObject {
public:
    /**
     * Starts the work thread
     */
    ActiveObject() : m_keepAlive(false) { startWork(); }

    /** 
     * Stops the work thread and finishes any pending jobs
     */
    virtual ~ActiveObject() { stopWork(); }

    /**
     * Checks if the work thread is running
     */
    bool isWorking() const { return m_keepAlive.load(); }

    /**
     * Starts the work thread if it is not already started
     */
    void startWork();
    
    /**
     * Stops the work thread if it is running, finishing any pending
     * jobs on the caller's thread.
     */
    void stopWork();
    
protected:

    /**
     * Submit a job. C must be copyable (a lambda is typical) with no function
     * arguments.
     */
    template<class C>
    void submitJob(C i_job) {
        LockGuard guard(m_mutex);
        m_work.emplace_back(i_job);
    }

private:
    
    using WorkQueue = std::list<std::function<void()>>;
    using LockGuard = std::unique_lock<std::mutex>;

    WorkQueue m_work;
    mutable std::mutex m_mutex;
    std::thread m_workThread;
    std::atomic_bool m_keepAlive;
};
}
