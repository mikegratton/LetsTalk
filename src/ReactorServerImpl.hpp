#include <chrono>

#include "PubSubType.hpp"
#include "Reactor.hpp"
#include "ReactorServer.hpp"

namespace lt {

namespace detail {
template <class Req, class Rep, class ProgressData>
class ReactorServerBackend : public std::enable_shared_from_this<ReactorServerBackend<Req, Rep, ProgressData>> {
   public:
    ReactorServerBackend(ParticipantPtr i_participant, std::string const& i_service)
        : m_participant(i_participant),
          m_replySender(m_participant->advertise<Rep>(reactorReplyName(i_service), "stateful", 8)),
          m_progressSender(m_participant->advertise<reactor_progress>(reactorProgressName(i_service), "stateful", 8)),
          m_service(i_service)
    {
        auto commandCallback = [this](reactor_command const&, Guid const& /*id*/, Guid const& relatedId) {
            LT_LOG << m_participant.get() << ":" << m_service << "-server"
                   << "  Session ID " << relatedId << " cancelled\n";
            LockGuard guard(m_sessionMutex);
            m_session.erase(relatedId);
        };
        m_participant->subscribe<reactor_command>(reactorCommandName(i_service), commandCallback, "stateful");
        auto reqCallback = [this](Req const& i_req, Guid const& id, Guid const& /*relatedId*/) {
            LockGuard guard(m_requestMutex);
            m_pending.emplace_back(id, i_req);
            guard.unlock();
            m_arePending.notify_one();
            LT_LOG << m_participant.get() << ":" << m_service << "-server"
                   << "  Request ID " << id << " enqued as pending\n";
        };
        m_participant->subscribe<Req>(reactorRequestName(m_service), reqCallback, "stateful");
    }

    ~ReactorServerBackend()
    {
        LockGuard guard(m_sessionMutex);
        m_participant->unsubscribe(reactorCommandName(m_service));
        m_participant->unsubscribe(reactorRequestName(m_service));
        m_participant->unadvertise(reactorReplyName(m_service));
        m_participant->unadvertise(reactorProgressName(m_service));
    }

    void logConnectionStatus() const
    {
        LT_LOG << m_participant.get() << " ";
        LT_LOG << "---- " << m_service << " Server ----\n";
        LT_LOG << "               Request pubs: " << m_participant->publisherCount(reactorRequestName(m_service))
               << "\n";
        LT_LOG << "               Command pubs: " << m_participant->publisherCount(reactorCommandName(m_service))
               << "\n";
        LT_LOG << "               Reply subs: " << m_participant->subscriberCount(reactorReplyName(m_service)) << "\n";
        LT_LOG << "               Progress subs: " << m_participant->subscriberCount(reactorProgressName(m_service))
               << "\n";
        LT_LOG << m_participant.get() << "\n";
    }

    bool havePendingSession() const
    {
        LockGuard guard(m_sessionMutex);
        return m_pending.size() > 0;
    }

    /// Mark that the reply was sent for session i_id
    void recordReply(Guid const& i_id)
    {
        reactor_progress message;
        message.progress(PROG_SUCCESS);
        m_progressSender.publish(message, m_progressSender.guid(), i_id);

        {
            LockGuard guard(m_sessionMutex);
            auto it = m_session.find(i_id);
            if (it != m_session.end()) { it->second = SUCCEED; }
        }
        LT_LOG << m_participant.get() << ":" << m_service << "-server"
               << "  Finish session ID " << i_id << "\n";
    }

    /// Send a reply for session i_id
    void reply(Guid const& i_id, Rep const& i_reply)
    {
        recordReply(i_id);
        m_replySender.publish(i_reply, m_replySender.guid(), i_id);
    }

    /// Get the next pending session and it's id
    std::pair<Guid, Req> getPendingSessionData(std::chrono::nanoseconds i_wait)
    {
        LockGuard guard(m_requestMutex);
        if (m_arePending.wait_for(guard, i_wait, [this]() { return !m_pending.empty(); })) {
            auto sessionData = std::move(m_pending.front());
            m_pending.pop_front();
            guard.unlock();
            LT_LOG << m_participant.get() << ":" << m_service << "-server"
                   << "  Starting new session ID " << sessionData.first << "\n";
            return sessionData;
        }
        return {Guid::UNKNOWN(), Req()};
    }

    /// Remove sessions that are finished, cancelled, or failed from the session map
    void pruneDeadSessions()
    {
        LockGuard guard(m_sessionMutex);
        for (auto it = m_session.begin(); it != m_session.end(); ++it) {
            switch (it->second) {
                case CANCELLED:
                case FAILED:
                case SUCCEED: m_session.erase(it);
                default:;
            }
        }
    }

    /// Send a progress update.
    /// @param i_payload -- this must already be serialized ProgressData
    void progress(Guid const& i_id, int i_progress, std::vector<unsigned char>& i_payload)
    {
        {
            LockGuard guard(m_sessionMutex);
            auto it = m_session.find(i_id);
            if (it != m_session.end()) {
                if (i_progress >= PROG_SUCCESS) {
                    it->second = SUCCEEDING;
                } else if (i_progress > PROG_START) {
                    it->second = RUNNING;
                } else if (i_progress == PROG_START) {
                    it->second = STARTING;
                } else {
                    m_session.erase(it);
                }
            } else if (i_progress == PROG_START) {
                m_session[i_id] = STARTING;
            }
        }
        LT_LOG << m_participant.get() << ":" << m_service << "-server"
               << "  Session ID " << i_id << " progress " << i_progress << " w/ " << i_payload.size()
               << " bytes data\n";
        reactor_progress message;
        message.progress(i_progress);
        message.data().swap(i_payload);
        m_progressSender.publish(message, m_progressSender.guid(), i_id);
    }

    /// Check if i_id is still live
    bool isAlive(Guid const& i_id) const
    {
        LockGuard guard(m_sessionMutex);
        auto it = m_session.find(i_id);
        if (it == m_session.end()) { return false; }
        switch (it->second) {
            case STARTING:
            case RUNNING: return true;
            default: return false;
        }
    }

    typename ReactorServer<Req, Rep, ProgressData>::Session getPendingSession(std::chrono::nanoseconds i_wait)
    {
        logConnectionStatus();
        auto s = std::move(getPendingSessionData(i_wait));
        if (s.first != Guid::UNKNOWN()) {
            return typename ReactorServer<Req, Rep, ProgressData>::Session(this->shared_from_this(),
                                                                           std::move(s.second), s.first);
        }
        return typename ReactorServer<Req, Rep, ProgressData>::Session(nullptr, Req(), Guid::UNKNOWN());
    }

    bool discoveredClients() const { return m_participant->subscriberCount(reactorReplyName(m_service)); }

    void disposeOfSession(Guid i_id)
    {
        LockGuard guard(m_sessionMutex);
        m_session.erase(i_id);
    }

   protected:
    enum State { STARTING, RUNNING, CANCELLING, CANCELLED, FAILING, FAILED, SUCCEEDING, SUCCEED };

    ParticipantPtr m_participant;  /// Participant used for all pubs and subs
    Publisher m_replySender;       /// Sends replys, ending the session
    Publisher m_progressSender;    /// Sends progress
    std::string m_service;         /// Service name

    ////////////////////////////////////////////////////////////
    // The session and pending data are guarded by m_sessionMutex
    using LockGuard = std::unique_lock<std::mutex>;
    mutable std::mutex m_sessionMutex;  /// Shared session guard
    mutable std::mutex m_requestMutex;
    std::condition_variable m_arePending;        /// Thread coordination for pending sessions
    std::map<Guid, State> m_session;             /// Map of id's to session statuses
    std::deque<std::pair<Guid, Req>> m_pending;  /// Pending sessions
};
}  // namespace detail

template <class Req, class Rep, class P>
ReactorServer<Req, Rep, P>::ReactorServer(std::shared_ptr<Backend> i_backend) : m_backend(i_backend)
{
}

template <class Req, class Rep, class P>
typename ReactorServer<Req, Rep, P>::Session ReactorServer<Req, Rep, P>::getPendingSession(
    std::chrono::nanoseconds i_wait)
{
    return m_backend->getPendingSession(i_wait);
}

template <class Req, class Rep, class ProgressData>
void ReactorServer<Req, Rep, ProgressData>::Session::reply(Rep const& i_reply)
{
    if (m_reactor) { m_reactor->reply(m_id, i_reply); }
}

template <class Req, class Rep, class ProgressData>
void ReactorServer<Req, Rep, ProgressData>::Session::progress(int i_progress, ProgressData const& i_data)
{
    if (m_reactor) {
        // Serialize data so that reactor_progress can be a generic type
        std::vector<unsigned char> truePayload;
        if (typeid(ProgressData) != typeid(reactor_void_progress)) {
            void* rawData = &const_cast<ProgressData&>(i_data);
            auto serSize = detail::PubSubType<ProgressData>{}.getSerializedSizeProvider(rawData)();
            truePayload.resize(serSize);
            efr::SerializedPayload_t payloadWrapper;
            payloadWrapper.data = truePayload.data();
            payloadWrapper.max_size = truePayload.size();
            detail::PubSubType<ProgressData> ser;
            ser.serialize(rawData, &payloadWrapper);
            payloadWrapper.data = nullptr;
        }
        m_reactor->progress(m_id, i_progress, truePayload);
    }
}

template <class Req, class Rep, class P>
void ReactorServer<Req, Rep, P>::Session::fail()
{
    if (nullptr == m_reactor) { return; }
    std::vector<unsigned char> noData;
    m_reactor->progress(m_id, PROG_FAILED, noData);
    m_reactor.reset();
}

template <class Req, class Rep, class P>
ReactorServer<Req, Rep, P>::Session::Session(std::shared_ptr<Backend> i_reactor, Req const& i_request, Guid i_id)
    : m_reactor(i_reactor), m_request(i_request), m_id(i_id)
{
    if (m_reactor) {
        std::vector<unsigned char> noData;
        m_reactor->progress(m_id, PROG_START, noData);
    }
}

template <class Req, class Rep, class P>
ReactorServer<Req, Rep, P>::Session::~Session()
{
    if (m_reactor) { m_reactor->disposeOfSession(m_id); }
}

template <class Req, class Rep, class ProgressData>
void ReactorServer<Req, Rep, ProgressData>::logConnectionStatus() const
{
    m_backend->logConnectionStatus();
}

template <class Req, class Rep, class ProgressData>
bool ReactorServer<Req, Rep, ProgressData>::Session::isAlive() const
{
    if (m_reactor) { return m_reactor->isAlive(m_id); }
    return false;
}

template <class Req, class Rep, class ProgressData>
bool ReactorServer<Req, Rep, ProgressData>::havePendingSession() const
{
    return m_backend->havePendingSession();
}

template <class Req, class Rep, class ProgressData>
int ReactorServer<Req, Rep, ProgressData>::discoveredClients() const
{
    return m_backend->discoveredClients();
}

}  // namespace lt