#pragma once
#include <queue>

#include "Reactor.hpp"
#include "ReactorClient.hpp"

namespace lt {
namespace detail {
template <class Req, class Rep, class ProgressData>
class ReactorClientBackend : public std::enable_shared_from_this<ReactorClientBackend<Req, Rep, ProgressData>> {
   public:
    Guid startNewSession(Req const& i_request)
    {
        Guid thisId;
        {
            LockGuard guard(m_mutex);
            thisId = m_lastId.increment();
            m_session[m_lastId];
        }
        m_requestSender.publish(i_request, thisId, Guid::UNKNOWN());
        LT_LOG << m_participant.get() << ":" << m_service << "-client"
               << "  Start new session ID " << thisId << "\n";
        return thisId;
    }

    typename ReactorClient<Req, Rep, ProgressData>::Session request(Req const& i_request)
    {
        logConnectionStatus();
        return typename ReactorClient<Req, Rep, ProgressData>::Session(this->shared_from_this(),
                                                                       startNewSession(i_request));
    }

    ReactorClientBackend(ParticipantPtr i_participant, std::string const& i_serviceName)
        : m_participant(i_participant),
          m_commandSender(i_participant->advertise<reactor_command>(reactorCommandName(i_serviceName), "stateful", -1)),
          m_service(i_serviceName)
    {
        auto progressLambda = [this](reactor_progress const& i_progress, Guid const& /*sampleId*/,
                                     Guid const& i_relatedId) {
            // Try to deserialize the sample
            bool haveData = false;
            ProgressData sample;
            if (i_progress.data().size() > 0) {
                PubSubType<ProgressData> deser;
                efr::SerializedPayload_t serialized;
                serialized.length = i_progress.data().size();
                serialized.data = const_cast<unsigned char*>(i_progress.data().data());
                serialized.encapsulation = CDR_LE;  // FIXME how do you know? It's in the encapsulation bytes, but
                                                    // we've got to dig through *OMG* documents for it :(
                haveData = deser.deserialize(&serialized, &sample);
                serialized.data = nullptr;  // Prevent double free, as mem is owned by rawData
            }
            // Place the sample
            LockGuard guard(m_mutex);
            auto it = m_session.find(i_relatedId);
            if (it == m_session.end()) { return; }
            it->second.progress = i_progress.progress();
            LT_LOG << m_participant.get() << ":" << m_service << "-client"
                   << "  Session ID " << i_relatedId << " progress " << i_progress.progress() << " w/ "
                   << i_progress.data().size() << " bytes data \n";
            if (i_progress.data().size() > 0) {
                if (haveData) {
                    it->second.progressData.push({i_progress.progress(), std::move(sample)});
                    it->second.cvProgressData.notify_one();
                } else {
                    LT_LOG << m_participant.get() << ":" << m_service << "-client"
                           << "  Session ID " << i_relatedId << " got mangled progress data\n";
                }
            }
        };
        m_participant->subscribe<reactor_progress>(reactorProgressName(i_serviceName), progressLambda, "stateful", -1);

        m_requestSender = m_participant->advertise<Req>(reactorRequestName(m_service), "stateful", -1);
        m_lastId = m_requestSender.guid();
        auto repCallback = [this](Rep const& i_reply, Guid const& /*sampleId*/, Guid const& i_relatedId) {
            LockGuard guard(m_mutex);
            auto it = m_session.find(i_relatedId);
            if (it == m_session.end()) { return; }
            LT_LOG << m_participant.get() << ":" << m_service << "-client"
                   << "  Finish session ID " << i_relatedId << "\n";
            it->second.progress = PROG_SUCCESS;
            it->second.reply = std::move(i_reply);
            it->second.replyReady = true;
            guard.unlock();
            it->second.cvReply.notify_one();
        };
        m_participant->subscribe<Rep>(reactorReplyName(m_service), repCallback, "stateful", -1);
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
        if (it == m_session.end()) { return false; }
        SessionData& sessionData = it->second;
        auto readyLambda = [&sessionData]() -> bool { return sessionData.replyReady; };
        if (sessionData.cvReply.wait_for(guard, i_wait, readyLambda)) {
            o_reply = std::move(sessionData.reply);
            return true;
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
        reactor_command command;
        command.command(Command::CANCEL);
        {
            LockGuard guard(m_mutex);
            m_session.erase(i_id);
        }
        m_commandSender.publish(command, Guid::UNKNOWN(), i_id);
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
        if (it == m_session.end()) { return false; }
        auto& session = it->second;
        auto waitLambda = [&session]() { return !session.progressData.empty(); };
        if (session.cvProgressData.wait_for(guard, i_wait, waitLambda)) {
            o_progress = std::move(session.progressData.top().second);
            session.progressData.pop();
            return true;
        }
        return false;
    }

    bool discoveredServer() const { return m_participant->subscriberCount(reactorRequestName(m_service)) > 0; }

    void disposeOfSession(Guid const& i_id)
    {
        LockGuard guard(m_mutex);
        m_session.erase(i_id);
    }

    ParticipantPtr m_participant;  /// Participant holding all pubs and subs
    Publisher m_requestSender;     /// Publisher for requests (that start sessions)
    Publisher m_commandSender;     /// Publisher for commands (i.e. cancelling a session)
    std::string m_service;         /// Service name used to compute topic names

    mutable std::mutex m_mutex;  /// Guards session data queue
    using LockGuard = std::unique_lock<std::mutex>;
    Guid m_lastId;  /// This is incremented with each session

    struct SessionData {
        using ProgressMark = std::pair<int, ProgressData>;
        struct LowestProgress {
            bool operator()(ProgressMark const& i_left, ProgressMark const& i_right)
            {
                return i_left.first > i_right.first;
            }
        };
        using ProgressQueue = std::priority_queue<ProgressMark, std::vector<ProgressMark>, LowestProgress>;

        int progress;                            /// Current progress mark level
        std::condition_variable cvProgressData;  /// Coordination for data in progressData queue
        ProgressQueue progressData;              /// Queue of all recieved but not retrieved progress data samples
        std::condition_variable cvReply;         /// Coordination for replyReady
        Rep reply;                               /// Reply data (only available at session end)
        bool replyReady;                         /// True when reply data is valid

        /// When SessionData is created, a request has been sent
        SessionData() : progress(PROG_SENT), replyReady(false) {}
    };

    std::map<Guid, SessionData> m_session;  /// Record of all active sessions indexed by id
};                                          // namespace detail
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
ReactorClient<Req, Rep, ProgressData>::Session::Session(std::shared_ptr<Backend> i_reactor, Guid i_id)
    : m_reactor(i_reactor), m_id(i_id)
{
}

template <class Req, class Rep, class ProgressData>
ReactorClient<Req, Rep, ProgressData>::Session::~Session()
{
    if (m_reactor) { m_reactor->disposeOfSession(m_id); }
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

template <class Req, class Rep, class ProgressData>
bool ReactorClient<Req, Rep, ProgressData>::discoveredServer() const
{
    return m_backend->discoveredServer();
}

}  // namespace lt
