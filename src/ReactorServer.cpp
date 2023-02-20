#include "ReactorServer.hpp"

namespace lt {
namespace detail {

void ReactorServerBackend::logConnectionStatus() const {
    LT_LOG << m_participant.get() << " ";
    LT_LOG << "---- " << m_service << " Server ----\n";
    LT_LOG << "               Request pubs: " << m_participant->publisherCount(reactorRequestName(m_service)) << "\n";
    LT_LOG << "               Command pubs: " << m_participant->publisherCount(reactorCommandName(m_service)) << "\n";
    LT_LOG << "               Reply subs: " << m_participant->subscriberCount(reactorReplyName(m_service)) << "\n";
    LT_LOG << "               Progress subs: " << m_participant->subscriberCount(reactorProgressName(m_service)) << "\n";
    LT_LOG << m_participant.get() << "\n";
}

void ReactorServerBackend::progress(Guid const& i_id, int i_progress) {
    LockGuard guard(m_mutex);
    auto it = m_session.find(i_id);
    if (it != m_session.end()) {
        if (i_progress >= PROG_SUCCESS) {
            it->second = SUCCEEDING;
        } else if (i_progress > PROG_START) {
            it->second = RUNNING;
        } else if (i_progress == PROG_START) {
            it->second = STARTING;
        } else {
            it->second = FAILED;
        }
    } else if (i_progress == PROG_START) {
        m_session[i_id] = STARTING;
    }
    LT_LOG << m_participant.get() << ":" << m_service << "-server"
           << "  Session ID " << i_id << " progress " << i_progress << "\n";
    std::unique_ptr<reactor_progress> message(new reactor_progress);
    message->progress(i_progress);
    m_progressSender.publish(std::move(message), m_progressSender.guid(), i_id);
}

bool ReactorServerBackend::isAlive(Guid const& i_id) const {
    LockGuard guard(m_mutex);
    auto it = m_session.find(i_id);
    if (it == m_session.end()) {
        return false;
    }
    switch (it->second) {
    case STARTING:
    case RUNNING:
        return true;
    default:
        return false;
    }
}

void ReactorServerBackend::recordReply(Guid const& i_id)
{
    progress(i_id, PROG_SUCCESS);
    LockGuard guard(m_mutex);
    auto it = m_session.find(i_id);
    if (it != m_session.end()) {
        it->second = SUCCEED;
    }
    LT_LOG << m_participant.get() << ":" << m_service << "-server"
           << "  Finish session ID " << i_id << "\n";
    
}
void ReactorServerBackend::pruneDeadSessions() {
    LockGuard guard(m_mutex);
    std::vector<Guid> dead;
    for (auto const& p : m_session) {
        switch (p.second) {
        case CANCELLED:
        case FAILED:
        case SUCCEED:
            dead.push_back(p.first);
        default:
            ;
        }
    }
    for (auto const& id : dead) {
        m_session.erase(id);
    }
}

bool ReactorServerBackend::havePendingSession() const {
    LockGuard guard(m_mutex);
    return m_pending.size() > 0;
}


ReactorServerBackend::ReactorServerBackend(ParticipantPtr i_participant, Publisher i_replySender, std::string i_service)
    : m_participant(i_participant)
    , m_replySender(i_replySender)
    , m_progressSender(i_participant->advertise<reactor_progress>(reactorProgressName(i_service)))
    , m_service(i_service) {
    i_participant->subscribeWithId<reactor_command>(reactorCommandName(i_service),
    [this](std::unique_ptr<reactor_command>, Guid, Guid id) { // Need to use related ID because entity codes don't match
        LT_LOG << m_participant.get() << ":" << m_service << "-server"
               << "  Session ID " << id << " cancelled\n";
        LockGuard guard(m_mutex);
        m_session[id] = CANCELLED;
    });
}

ReactorServerBackend::~ReactorServerBackend() {
    LockGuard guard(m_mutex);
    m_participant->unsubscribe(reactorCommandName(m_service));
    m_participant->unsubscribe(reactorRequestName(m_service));
    m_participant->unadvertise(reactorReplyName(m_service));
    m_participant->unadvertise(reactorProgressName(m_service));
}

}
}
