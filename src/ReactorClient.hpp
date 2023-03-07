#pragma once
#include "LetsTalk.hpp"
#include "Reactor.hpp"
#include <mutex>
#include <future>

namespace lt {

namespace detail {
class ReactorClientBackend;
}

template<class Req, class Rep, class ProgressData=reactor_void_progress>
class ReactorClient {
public:

    /**
     * Construct a reactor client that makes requests to i_serviceName
     * @param i_participant Participant to use for communications
     * @param i_serviceName Name of this service 
     */
    ReactorClient(ParticipantPtr i_participant, std::string const& i_serviceName);

    /**
     * Each session is modeled by this class. It is lightweight and copyable.
     */
    class Session {
    public:
        
        /// Get the current progress (0 to 100)
        int progress() const;
        
        // Get the progress data. All data sent is queued.
        std::unique_ptr<ProgressData> progressData(std::chrono::nanoseconds const& i_wait = std::chrono::nanoseconds(0));

        /// Check if this session is alive
        bool isAlive() const;

        /// Cancel the request
        void cancel();

        /// Get the response. Will return nullptr if the response is not ready by i_wait
        std::unique_ptr<Rep> get(std::chrono::nanoseconds const& i_wait = std::chrono::nanoseconds(0));

    protected:
        friend class ReactorClient;
        Session(std::shared_ptr<detail::ReactorClientBackend> i_backend, Guid i_id);

        std::shared_ptr<detail::ReactorClientBackend> m_backend;
        Guid m_id;
    };

    /// Make a request to the reactor service
    Session request(std::unique_ptr<Req> i_request);

    /// Logs debug information about the connections if LT_VERBOSE is defined
    void logConnectionStatus() const;

protected:
    std::shared_ptr<detail::ReactorClientBackend> m_backend;
};

//////////////////////////////////////////////////////////////////////////////
// Implementation details follow

namespace detail {

/// Generic backend for all clients
class ReactorClientBackend {
public:
    void logConnectionStatus() const;

    void cancel(Guid const& i_id);

    bool isAlive(Guid const& i_id);

    int progress(Guid const& i_id);

    std::unique_ptr<reactor_progress> progressData(Guid const& i_id, std::chrono::nanoseconds i_wait);

    template<class Req, class Rep>
    void connect() {
        m_requestSender = m_participant->advertise<Req>(reactorRequestName(m_service));
        m_lastId = m_requestSender.guid();
        m_participant->subscribeWithId<Rep>(reactorReplyName(m_service),
        [this](std::unique_ptr<Rep> i_reply, Guid /*sampleId*/, Guid i_relatedId) {
            LockGuard guard(m_mutex);
            auto it = m_session.find(i_relatedId);
            if (it != m_session.end()) {
                LT_LOG << m_participant.get() << ":" << m_service
                       << "-client" << "  Finish session ID " << i_relatedId << "\n";
                void* ptr = i_reply.release();
                it->second.promise->set_value(ptr);
                it->second.deleter = [ptr]() { delete (Rep*) ptr; };
            }
        });
    }

    template<class Req>
    Guid startNewSession(std::unique_ptr<Req> i_request) {
        LockGuard guard(m_mutex);
        m_lastId.increment();
        m_requestSender.publish(std::move(i_request), m_lastId, Guid::UNKNOWN());
        m_session[m_lastId];
        LT_LOG << m_participant.get() << ":" << m_service << "-client"
               << "  Start new session ID " << m_lastId << "\n";
        return m_lastId;
    }

    void* get(Guid const& i_id, std::chrono::nanoseconds const& i_wait) ;

    ReactorClientBackend(ParticipantPtr i_participant, std::string const& i_serviceName);

    ~ReactorClientBackend();

protected:

    ParticipantPtr m_participant;
    Publisher m_requestSender;
    Publisher m_commandSender;
    std::string m_service;

    mutable std::mutex m_mutex;
    using LockGuard = std::unique_lock<std::mutex>;
    Guid m_lastId;

    struct SessionData {
        int progress;
        ThreadSafeQueue<reactor_progress> progressData;
        std::shared_ptr<std::promise<void*>> promise;
        std::future<void*> replyFuture;
        std::function<void(void)> deleter;

        SessionData()
            : progress(PROG_SENT)
            , promise(std::make_shared<std::promise<void*>>())
            , replyFuture(promise->get_future())
        { }
        ~SessionData() { if (deleter) deleter(); }
    };

    std::map<Guid, SessionData> m_session;
};

}

template<class Req, class Rep, class ProgressData>
ReactorClient<Req, Rep, ProgressData>::ReactorClient(ParticipantPtr i_participant, std::string const& i_serviceName) {
    m_backend = std::make_shared<detail::ReactorClientBackend>(i_participant, i_serviceName);
    m_backend->connect<Req, Rep>();
}


template<class Req, class Rep, class ProgressData>
typename ReactorClient<Req, Rep, ProgressData>::Session ReactorClient<Req, Rep, ProgressData>::request(std::unique_ptr<Req> i_request) {
    logConnectionStatus();
    return Session(m_backend, m_backend->startNewSession(std::move(i_request)));
}

template<class Req, class Rep, class ProgressData>
void ReactorClient<Req,Rep,ProgressData>::logConnectionStatus() const { m_backend->logConnectionStatus(); }

template<class Req, class Rep, class ProgressData>
int ReactorClient<Req, Rep, ProgressData>::Session::progress() const {
    return m_backend->progress(m_id);
}

template<class Req, class Rep, class ProgressData>
std::unique_ptr<ProgressData> ReactorClient<Req, Rep, ProgressData>::Session::progressData(std::chrono::nanoseconds const& i_wait)
{
    auto progressDataPtr = m_backend->progressData(m_id, i_wait);
    if (progressDataPtr) {
        std::unique_ptr<ProgressData> data(new ProgressData);
        PubSubType<ProgressData> deser;
        efr::SerializedPayload_t rawData; 
        rawData.length = progressDataPtr->data().size();
        rawData.data = progressDataPtr->data().data();
        if (rawData.length == 0) {
            return nullptr;
        }
        rawData.encapsulation = CDR_LE; // FIXME how do you know?
        deser.deserialize(&rawData, data.get()); 
        rawData.data = nullptr; // Prevent double free
        return data;
    }
    return nullptr;
}

template<class Req, class Rep, class ProgressData>
bool ReactorClient<Req, Rep, ProgressData>::Session::isAlive() const {
    return m_backend->isAlive(m_id);
}

template<class Req, class Rep, class ProgressData>
void ReactorClient<Req, Rep, ProgressData>::Session::cancel() {
    m_backend->cancel(m_id);
}

template<class Req, class Rep, class ProgressData>
std::unique_ptr<Rep> ReactorClient<Req, Rep, ProgressData>::Session::get(std::chrono::nanoseconds const& i_wait) {
    void* rawPtr = m_backend->get(m_id, i_wait);
    return std::unique_ptr<Rep>((Rep*) rawPtr);
    
}

template<class Req, class Rep, class ProgressData>
ReactorClient<Req, Rep, ProgressData>::Session::Session(std::shared_ptr<detail::ReactorClientBackend> i_backend, Guid i_id)
    : m_backend(i_backend)
    , m_id(i_id) {
}

}
