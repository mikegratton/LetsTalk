#include "ActiveObject.hpp"

namespace lt {
namespace {
const std::chrono::milliseconds POLL_INTERVAL(4);
}

void ActiveObject::startWork()
{
    if (m_keepAlive) { return; }
    m_keepAlive = true;
    m_workThread = std::thread([this]() {
        // Poll the queue, running jobs
        while (m_keepAlive) {
            LockGuard guard(m_mutex);
            if (!m_work.empty()) {
                m_work.front()();
                m_work.pop_front();
            } else {
                guard.unlock();
                std::this_thread::sleep_for(POLL_INTERVAL);
            }
        }

        // Drain queue
        LockGuard guard(m_mutex);
        while (!m_work.empty()) {
            m_work.front()();
            m_work.pop_front();
        }
    });
}

void ActiveObject::stopWork()
{
    m_keepAlive = false;
    if (m_workThread.joinable()) { m_workThread.join(); }
}

}  // namespace lt
