#pragma once
#include "LetsTalk.hpp"
#include "Reactor.h"
#include "Reactor.hpp"
namespace lt {

namespace detail {
class ReactorServerBackend;
}

template<class Req, class Rep, class ProgressData = reactor_void_progress>
class ReactorServer {
public:
    class Session {
    public:
        /// Inspect the request data for this session
        Req const* request() const { return m_request.get(); }

        /// Transmit progress information to the client
        void progress(int i_progress, std::unique_ptr<ProgressData> i_data = nullptr);

        /// Check that the client hasn't cancelled this session, or that it isn't already complete
        bool isAlive() const;
        
        /// Send the reply, ending the session
        void reply(std::unique_ptr<Rep> i_reply);
        
        /// Mark the request as failed
        void fail();
        
    protected:

        friend class ReactorServer;
        
        Session(std::shared_ptr<detail::ReactorServerBackend> i_backend, std::shared_ptr<Req> i_request, Guid i_id);
        
        std::shared_ptr<detail::ReactorServerBackend> m_backend;
        std::shared_ptr<Req> m_request;
        Guid m_id;
    };


    ReactorServer(ParticipantPtr m_participant, std::string const& i_service);

    /// Check if there is a pending session
    bool havePendingSession() const;

    /// Get the next pending request as a new session object. If there are no pending requests,
    /// this retuns a session object where isAlive() returns false
    Session getPendingSession(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0));

    /// For debugging. Prints connection status if LT_VERBOSE is defined
    void logConnectionStatus() const;
    
protected:
    std::shared_ptr<detail::ReactorServerBackend> m_backend;
};

/////////////////////////////////////////////////////////////////////////
// Implementation details follow

namespace detail {

    /*
     * Erase the request type, currying the deleter. This allows the backend to 
     * be generic among all ReactorServers
     */
struct TypeErasedRequest {
    Guid id;
    void* requestData;

    template<class T>
    std::unique_ptr<T> take() {
        std::unique_ptr<T> cast((T*) requestData);
        requestData = nullptr;
        return cast;
    }

    template<class T>
    TypeErasedRequest(Guid const& i_id, std::unique_ptr<T> i_request)
        : id(i_id)
        , requestData(i_request.release())
        , deleter([this]() { delete (T*) requestData; }) {
    }

    TypeErasedRequest() : requestData(nullptr) {}

    ~TypeErasedRequest() { if (requestData) deleter(); }

private:
    std::function<void(void)> deleter;
};

class ReactorServerBackend {
public:
    void logConnectionStatus() const;

    void progress(Guid const& i_id, int i_progress, efr::SerializedPayload_t& i_payload);

    bool isAlive(Guid const& i_id) const;

    template<class Rep>
    void reply(Guid const& i_id, std::unique_ptr<Rep> i_reply);

    void pruneDeadSessions();

    bool havePendingSession() const;

    template<class Req>
    std::pair<Guid, std::unique_ptr<Req>> getPendingSession(std::chrono::nanoseconds i_wait);

    template<class Req>
    void subscribeToRequest();
    
    ReactorServerBackend(ParticipantPtr i_participant, Publisher i_replySender, std::string i_service);

    ~ReactorServerBackend();

protected:

    void recordReply(Guid const& i_id);

    enum State {
        STARTING,
        RUNNING,
        CANCELLING,
        CANCELLED,
        FAILING,
        FAILED,
        SUCCEEDING,
        SUCCEED
    };

    ParticipantPtr m_participant;
    Publisher m_replySender; // is type erased already
    Publisher m_progressSender;
    std::string m_service;

    ////////////////////////////////////////////////////////////
    // The session and pending data are guarded by m_mutex
    using LockGuard = std::unique_lock<std::mutex>;
    mutable std::mutex m_mutex;
    std::condition_variable m_arePending;
    std::map<Guid, State> m_session;
    std::deque<TypeErasedRequest> m_pending;
};


template<class Rep>
void ReactorServerBackend::reply(Guid const& i_id, std::unique_ptr<Rep> i_reply) {
    recordReply(i_id);
    m_replySender.publish(std::move(i_reply), m_replySender.guid(), i_id);
}

template<class Req>
std::pair<Guid, std::unique_ptr<Req>> ReactorServerBackend::getPendingSession(std::chrono::nanoseconds i_wait) {
    LockGuard guard(m_mutex);
    if (m_arePending.wait_for(guard, i_wait, [this]() { return !m_pending.empty(); })) {
        auto id = m_pending.front().id;
        std::unique_ptr<Req> request = m_pending.front().take<Req>();
        m_pending.pop_front();
        LT_LOG << m_participant.get() << ":" << m_service << "-server"
            << "  Starting new session ID " << id << "\n";
        return {id, std::move(request)};
    }
    return {Guid::UNKNOWN(), nullptr};
}

template<class Req>
void ReactorServerBackend::subscribeToRequest() {
    m_participant->subscribeWithId<Req>(reactorRequestName(m_service),
    [this](std::unique_ptr<Req> i_req, Guid id, Guid /*relatedId*/) {
        LockGuard guard(m_mutex);
        m_pending.emplace_back(id, std::move(i_req));
        LT_LOG << m_participant.get() << ":" << m_service << "-server"
               << "  Request ID " << id << " enqued as pending\n";
    });
}
}

template<class Req, class Rep, class P>
bool ReactorServer<Req,Rep,P>::havePendingSession() const {
    return m_backend->havePendingSession();
}

template<class Req, class Rep, class P>
typename ReactorServer<Req,Rep,P>::Session ReactorServer<Req,Rep, P>::getPendingSession(std::chrono::nanoseconds i_wait) {
    logConnectionStatus();
    m_backend->pruneDeadSessions();    
    auto s = m_backend->getPendingSession<Req>(i_wait);
    if (s.second) {
        return Session(m_backend, std::move(s.second), s.first);
    }
    return Session(nullptr, nullptr, Guid::UNKNOWN());
}

template<class Req, class Rep, class P>
void ReactorServer<Req,Rep,P>::logConnectionStatus() const { m_backend->logConnectionStatus(); }

template<class Req, class Rep, class ProgressData>
void ReactorServer<Req,Rep, ProgressData>::Session::progress(int i_progress, std::unique_ptr<ProgressData> i_data) {
    if (m_backend) {
        efr::SerializedPayload_t payload;
        if ( i_data && typeid(ProgressData) != typeid(reactor_void_progress)){
            PubSubType<ProgressData> ser;
            ser.serialize(i_data.get(), &payload);
        }
        m_backend->progress(m_id, i_progress, payload);
    }
}

template<class Req, class Rep, class P>
bool ReactorServer<Req,Rep,P>::Session::isAlive() const {
    return (m_backend && m_backend->isAlive(m_id));
}

template<class Req, class Rep, class P>
void ReactorServer<Req,Rep,P>::Session::reply(std::unique_ptr<Rep> i_reply) {
    if (nullptr == m_backend) {
        return;
    }
    m_backend->reply(m_id, std::move(i_reply));
    m_backend.reset();
}

template<class Req, class Rep, class P>
void ReactorServer<Req,Rep,P>::Session::fail() {
    if (nullptr == m_backend) {
        return;
    }
    progress(PROG_FAILED);
    m_backend.reset();
}

template<class Req, class Rep, class P>
ReactorServer<Req, Rep,P>::ReactorServer(ParticipantPtr i_participant, std::string const& i_service)
    : m_backend(std::make_shared<detail::ReactorServerBackend>(i_participant,
                i_participant->advertise<Rep>(reactorReplyName(i_service)),
                i_service)) {
    m_backend->subscribeToRequest<Req>();
}

template<class Req, class Rep, class P>
ReactorServer<Req, Rep, P>::Session::Session(std::shared_ptr<detail::ReactorServerBackend> i_backend, 
                                          std::shared_ptr<Req> i_request, Guid i_id)
            : m_backend(i_backend)
            , m_request(i_request)
            , m_id(i_id) {
            if (m_backend) {
                progress(PROG_START);
            }
        }

}
