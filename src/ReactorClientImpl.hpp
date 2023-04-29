#pragma once
#include "Reactor.hpp"
#include "ReactorClient.hpp"

namespace lt {
namespace detail {
template <class Req, class Rep, class ProgressData>
class ReactorClientBackend : public std::enable_shared_from_this<ReactorClientBackend<Req, Rep, ProgressData>> {
   public:
    Guid startNewSession(Req const& i_request)
    {
        LockGuard guard(m_mutex);
        m_lastId.increment();
        m_requestSender.publish(i_request, m_lastId, Guid::UNKNOWN());
        m_session[m_lastId];
        LT_LOG << m_participant.get() << ":" << m_service << "-client"
               << "  Start new session ID " << m_lastId << "\n";
        return m_lastId;
    }

    typename ReactorClient<Req, Rep, ProgressData>::Session request(Req const& i_request)
    {
        logConnectionStatus();
        return typename ReactorClient<Req, Rep, ProgressData>::Session(this->shared_from_this(),
                                                                       startNewSession(i_request));
    }

    ReactorClientBackend(ParticipantPtr i_participant, std::string const& i_serviceName)
        : m_participant(i_participant),
          m_commandSender(i_participant->advertise<reactor_command>(reactorCommandName(i_serviceName))),
          m_service(i_serviceName)
    {
        auto progressLambda = [this](reactor_progress const& i_progress, Guid const& /*sampleId*/,
                                     Guid const& i_relatedId) {
            LockGuard guard(m_mutex);
            auto it = m_session.find(i_relatedId);
            if (it != m_session.end()) {
                LT_LOG << m_participant.get() << ":" << m_service << "-client"
                       << "  Session ID " << i_relatedId << " progress " << i_progress.progress() << "\n";
                it->second.progress = i_progress.progress();
                it->second.progressData.push(i_progress);
            }
        };
        m_participant->subscribe<reactor_progress>(reactorProgressName(i_serviceName), progressLambda);

        m_requestSender = m_participant->advertise<Req>(reactorRequestName(m_service));
        m_lastId = m_requestSender.guid();
        auto repCallback = [this](Rep const& i_reply, Guid const& /*sampleId*/, Guid const& i_relatedId) {
            LockGuard guard(m_mutex);
            auto it = m_session.find(i_relatedId);
            if (it != m_session.end()) {
                LT_LOG << m_participant.get() << ":" << m_service << "-client"
                       << "  Finish session ID " << i_relatedId << "\n";
                it->second.reply = std::move(i_reply);
                it->second.replyReady = true;
            }
        };
        m_participant->subscribe<Rep>(reactorReplyName(m_service), repCallback);
    }

    ~ReactorClientBackend()
    {
        LockGuard guard(m_mutex);
        m_participant->unadvertise(reactorCommandName(m_service));
        m_participant->unadvertise(reactorRequestName(m_service));
        m_participant->unsubscribe(reactorReplyName(m_service));
        m_participant->unsubscribe(reactorProgressName(m_service));
    }

    bool get(Rep& o_reply, Guid const& i_id, std::chrono::nanoseconds const& i_wait)
    {
        LockGuard guard(m_mutex);
        auto it = m_session.find(i_id);
        if (it != m_session.end()) {
            SessionData& sessionData = it->second;
            auto readyLambda = [&sessionData]() -> bool { return sessionData.replyReady; };
            if (sessionData.cvReply.wait_for(guard, i_wait, readyLambda)) {
                o_reply = std::move(sessionData.reply);
                m_session.erase(it);
                return true;
            }
        }
        return false;
    }

    void logConnectionStatus() const
    {
        LT_LOG << m_participant.get() << " ";
        LT_LOG << "---- " << m_service << " Client ----\n";
        LT_LOG << "               Request subs: " << m_participant->subscriberCount(reactorRequestName(m_service))
               << "\n";
        LT_LOG << "               Command subs: " << m_participant->subscriberCount(reactorCommandName(m_service))
               << "\n";
        LT_LOG << "               Reply pubs: " << m_participant->publisherCount(reactorReplyName(m_service)) << "\n";
        LT_LOG << "               Progress pubs: " << m_participant->publisherCount(reactorProgressName(m_service))
               << "\n";
        LT_LOG << m_participant.get() << "\n";
    }

    void cancel(Guid const& i_id)
    {
        LockGuard guard(m_mutex);
        auto command = std::unique_ptr<reactor_command>(new reactor_command);
        command->command(Command::CANCEL);
        m_commandSender.publish(std::move(command), Guid::UNKNOWN(), i_id);
        m_session.erase(i_id);
        LT_LOG << m_participant.get() << ":" << m_service << "-client"
               << "  Cancelled session ID " << i_id << "\n";
    }

    bool isAlive(Guid const& i_id) const
    {
        LockGuard guard(m_mutex);
        auto it = m_session.find(i_id);
        if (it != m_session.end()) { return (it->second.progress >= 0 && it->second.progress < 100); }
        return false;
    }

    int progress(Guid const& i_id) const
    {
        LockGuard guard(m_mutex);
        auto it = m_session.find(i_id);
        if (it != m_session.end()) { return it->second.progress; }
        return PROG_UNKNOWN;
    }

    bool progressData(ProgressData& o_progress, Guid const& i_id, std::chrono::nanoseconds i_wait)
    {
        LockGuard guard(m_mutex);
        auto it = m_session.find(i_id);
        if (it != m_session.end()) {
            auto data = it->second.progressData.pop(i_wait);
            if (data) {
                PubSubType<ProgressData> deser;
                efr::SerializedPayload_t serialized;
                serialized.length = data->data().size();
                serialized.data = data->data().data();
                serialized.encapsulation = CDR_LE;  // FIXME how do you know?
                deser.deserialize(&serialized, &o_progress);
                serialized.data = nullptr;  // Prevent double free, as mem is owned by rawData
                return true;
            }
        }
        return false;
    }

    ParticipantPtr m_participant;  /// Participant holding all pubs and subs
    Publisher m_requestSender;     /// Publisher for requests (that start sessions)
    Publisher m_commandSender;     /// Publisher for commands (i.e. cancelling a session)
    std::string m_service;         /// Service name used to compute topic names

    mutable std::mutex m_mutex;  /// Guards session data queue
    using LockGuard = std::unique_lock<std::mutex>;
    Guid m_lastId;  /// This is incremented with each session

    struct SessionData {
        int progress;  /// Current progress mark
        ThreadSafeQueue<reactor_progress>
            progressData;  /// Queue of all recieved but not retrieved progress data samples
        std::condition_variable cvReply;
        Rep reply;
        bool replyReady;

        /// When SessionData is created, a request has been sent
        SessionData() : progress(PROG_SENT), replyReady(false) {}
    };

    std::map<Guid, SessionData> m_session;  /// Record of all active sessions indexed by id
};
}  // namespace detail

template <class Req, class Rep, class ProgressData>
ReactorClient<Req, Rep, ProgressData>::ReactorClient(std::shared_ptr<Backend> i_backend) : m_backend(i_backend)
{
}

template <class Req, class Rep, class ProgressData>
typename ReactorClient<Req, Rep, ProgressData>::Session ReactorClient<Req, Rep, ProgressData>::request(
    Req const& i_request)
{
    return m_backend->request(i_request);
}

template <class Req, class Rep, class ProgressData>
void ReactorClient<Req, Rep, ProgressData>::logConnectionStatus() const
{
    m_backend->logConnectionStatus();
}

template <class Req, class Rep, class ProgressData>
bool ReactorClient<Req, Rep, ProgressData>::Session::progressData(ProgressData& o_progress,
                                                                  std::chrono::nanoseconds const& i_wait)
{
    if (m_reactor->progressData(o_progress, m_id, i_wait)) { return true; }
    return false;
}

template <class Req, class Rep, class ProgressData>
bool ReactorClient<Req, Rep, ProgressData>::Session::get(Rep& o_reply, std::chrono::nanoseconds const& i_wait)
{
    return m_reactor->get(o_reply, m_id, i_wait);
}

template <class Req, class Rep, class ProgressData>
ReactorClient<Req, Rep, ProgressData>::Session::Session(std::shared_ptr<Backend> i_reactor, Guid i_id)
    : m_reactor(i_reactor), m_id(i_id)
{
}

template <class Req, class Rep, class ProgressData>
void ReactorClient<Req, Rep, ProgressData>::Session::cancel()
{
    if (m_reactor) { m_reactor->cancel(m_id); }
}

template <class Req, class Rep, class ProgressData>
bool ReactorClient<Req, Rep, ProgressData>::Session::isAlive() const
{
    if (m_reactor) { return m_reactor->isAlive(m_id); }
    return false;
}

template <class Req, class Rep, class ProgressData>
int ReactorClient<Req, Rep, ProgressData>::Session::progress() const
{
    if (m_reactor) { return m_reactor->progress(m_id); }
    return PROG_UNKNOWN;
}

}  // namespace lt
