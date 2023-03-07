#include "ReactorClient.hpp"

namespace lt {
namespace detail {

ReactorClientBackend::ReactorClientBackend(ParticipantPtr i_participant, std::string const& i_serviceName)
    : m_participant(i_participant)
    , m_commandSender(i_participant->advertise<reactor_command>(reactorCommandName(i_serviceName)))
    , m_service(i_serviceName) {
    i_participant->subscribeWithId<reactor_progress>(reactorProgressName(i_serviceName),
    [this](std::unique_ptr<reactor_progress> i_progress, Guid /*sampleId*/, Guid i_relatedId) {
        LockGuard guard(m_mutex);
        auto it = m_session.find(i_relatedId);
        if (it != m_session.end()) {
            LT_LOG << m_participant.get() << ":" << m_service << "-client"
                   << "  Session ID " << i_relatedId << " progress " << i_progress->progress() << "\n";
            it->second.progress = i_progress->progress();
            it->second.progressData.push(std::move(i_progress));
        }
    });
}


ReactorClientBackend::~ReactorClientBackend() {
    LockGuard guard(m_mutex);
    m_participant->unadvertise(reactorCommandName(m_service));
    m_participant->unadvertise(reactorRequestName(m_service));
    m_participant->unsubscribe(reactorReplyName(m_service));
    m_participant->unsubscribe(reactorProgressName(m_service));
}


void* ReactorClientBackend::get(Guid const& i_id, std::chrono::nanoseconds const& i_wait) {
    LockGuard guard(m_mutex);
    auto it = m_session.find(i_id);
    if (it != m_session.end()) {
        if (it->second.replyFuture.wait_for(i_wait) == std::future_status::ready) {
            auto rep = it->second.replyFuture.get();
            it->second.deleter = [](){};
            m_session.erase(it);
            return rep;
        }
    }
    return nullptr;
}


void ReactorClientBackend::logConnectionStatus() const {
    LT_LOG << m_participant.get() << " ";
    LT_LOG << "---- " << m_service << " Client ----\n";
    LT_LOG << "               Request subs: " << m_participant->subscriberCount(reactorRequestName(m_service)) << "\n";
    LT_LOG << "               Command subs: " << m_participant->subscriberCount(reactorCommandName(m_service)) << "\n";
    LT_LOG << "               Reply pubs: " << m_participant->publisherCount(reactorReplyName(m_service)) << "\n";
    LT_LOG << "               Progress pubs: " << m_participant->publisherCount(reactorProgressName(m_service)) << "\n";
    LT_LOG << m_participant.get() << "\n";
}

void ReactorClientBackend::cancel(Guid const& i_id) {
    LockGuard guard(m_mutex);
    auto command = std::unique_ptr<reactor_command>(new reactor_command);
    command->command(Command::CANCEL);
    m_commandSender.publish(std::move(command), Guid::UNKNOWN(), i_id);
    m_session.erase(i_id);
    LT_LOG << m_participant.get() << ":" << m_service << "-client"
           << "  Cancelled session ID " << i_id << "\n";
}

bool ReactorClientBackend::isAlive(Guid const& i_id) {
    LockGuard guard(m_mutex);
    auto it = m_session.find(i_id);
    if (it != m_session.end()) {
        return (it->second.progress >= 0 && it->second.progress < 100);
    }
    return false;
}

int ReactorClientBackend::progress(Guid const& i_id) {
    LockGuard guard(m_mutex);
    auto it = m_session.find(i_id);
    if (it != m_session.end()) {
        return it->second.progress;
    }
    return PROG_UNKNOWN;
}

std::unique_ptr<reactor_progress> ReactorClientBackend::progressData(Guid const& i_id, std::chrono::nanoseconds i_wait)
{
    LockGuard guard(m_mutex);
    auto it = m_session.find(i_id);
    if (it != m_session.end()) {
        return it->second.progressData.pop(i_wait);
    }
    return nullptr;
}

}
}
