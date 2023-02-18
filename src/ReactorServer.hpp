#pragma once
#include "LetsTalk.hpp"
#include "Reactor.h"
#include "Reactor.hpp"
namespace lt {
/*
 * OPTIONS
 * 1. Pull. API for getting an action session that is used to fill the request
 * 2. Push. API has callbacks that drive session creation
 */

/*
 * TODO generic void* backend
 */

template<class Req, class Rep>
class ReactorServer {
protected: struct Backend;
public:
    class Session {
    public:
        Req const* request() const { return m_request.get(); }

        void progress(int i_progress);

        bool isAlive() const;
        void reply(std::unique_ptr<Rep> i_reply);
        void fail();

        Session(std::shared_ptr<Backend> i_backend, std::unique_ptr<Req> i_request, Guid i_id)
            : m_backend(i_backend)
            , m_request(std::move(i_request))
            , m_id(i_id) {
            progress(PROG_START);
        }
    protected:
        
        std::shared_ptr<Backend> m_backend;
        std::unique_ptr<Req> m_request;
        Guid m_id;
    };


    ReactorServer(ParticipantPtr m_participant, std::string const& i_service);

    bool havePendingSession() const {
        return m_backend->havePendingSession();
    }

    Session getPendingSession() {
        logConnectionStatus();
        m_backend->pruneDeadSessions();
        if (m_backend->m_pending.empty()) {            
            return Session(nullptr, nullptr, Guid::UNKNOWN());
        }
        auto s = m_backend->getPendingSession();                
        return Session(m_backend, std::move(s.second), s.first);

    }
    
    void logConnectionStatus() const { m_backend->logConnectionStatus(); }

protected:

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

    struct Backend {
        
        void logConnectionStatus() const {
            LT_LOG << m_participant.get() << " ";
            LT_LOG << "---- " << m_service << " Server ----\n";
            LT_LOG << "               Request pubs: " << m_participant->publisherCount(reactorRequestName(m_service)) << "\n";
            LT_LOG << "               Command pubs: " << m_participant->publisherCount(reactorCommandName(m_service)) << "\n";
            LT_LOG << "               Reply subs: " << m_participant->subscriberCount(reactorReplyName(m_service)) << "\n";
            LT_LOG << "               Progress subs: " << m_participant->subscriberCount(reactorProgressName(m_service)) << "\n";
            LT_LOG << m_participant.get() << "\n";
        }

        void progress(Guid const& i_id, int i_progress) {
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

        bool isAlive(Guid const& i_id) const {
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

        void reply(Guid const& i_id, std::unique_ptr<Rep> i_reply) {
            progress(i_id, PROG_SUCCESS);
            LockGuard guard(m_mutex);
            auto it = m_session.find(i_id);
            if (it != m_session.end()) {
                it->second = SUCCEED;
            }
            LT_LOG << m_participant.get() << ":" << m_service << "-server"
            << "  Finish session ID " << i_id << "\n";
            m_replySender.publish(std::move(i_reply), m_replySender.guid(), i_id);
        }

        void pruneDeadSessions() {
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

        bool havePendingSession() const {
            LockGuard guard(m_mutex);
            return m_pending.size() > 0;
        }
        
        std::pair<Guid, std::unique_ptr<Req>> getPendingSession()
        {
            LockGuard guard(m_mutex);
            auto session = std::move(m_pending.front());
            m_pending.pop_front();  
            LT_LOG << m_participant.get() << ":" << m_service << "-server"
            << "  Starting new session ID " << session.first << "\n";
            return session;
        }

        Backend(ParticipantPtr i_participant, std::string i_service) 
        : m_participant(i_participant)
        , m_replySender(i_participant->advertise<Rep>(reactorReplyName(i_service)))
        , m_progressSender(i_participant->advertise<reactor_progress>(reactorProgressName(i_service)))
        , m_service(i_service)
        {
            i_participant->subscribeWithId<Req>(reactorRequestName(i_service),
            [this](std::unique_ptr<Req> i_req, Guid id, Guid /*relatedId*/) {
                LockGuard guard(m_mutex);                
                m_pending.emplace_back(id, std::move(i_req));
                LT_LOG << m_participant.get() << ":" << m_service << "-server" 
                << "  Request ID " << id << " enqued as pending\n";
            });
            i_participant->subscribeWithId<reactor_command>(reactorCommandName(i_service),
            [this](std::unique_ptr<reactor_command>, Guid id, Guid /*relatedId*/) {
                LockGuard guard(m_mutex);
                m_session[id] = CANCELLED;
                LT_LOG << m_participant.get() << ":" << m_service << "-server"
                << "  Session ID " << id << " cancelled\n";
            });
        }
        
        ~Backend()
        {
            LockGuard guard(m_mutex);
            m_participant->unsubscribe(reactorCommandName(m_service));
            m_participant->unsubscribe(reactorRequestName(m_service));
            m_participant->unadvertise(reactorReplyName(m_service));
            m_participant->unadvertise(reactorProgressName(m_service));
        }

        ParticipantPtr m_participant;
        Publisher m_replySender;
        Publisher m_progressSender;
        std::string m_service;
        
        ////////////////////////////////////////////////////////////
        // The session and pending data are guarded by m_mutex
        using LockGuard = std::unique_lock<std::mutex>;
        mutable std::mutex m_mutex;        
        std::map<Guid, State> m_session;
        std::deque<std::pair<Guid, std::unique_ptr<Req>>> m_pending;
    };
    std::shared_ptr<Backend> m_backend;    
};

template<class Req, class Rep>
void ReactorServer<Req,Rep>::Session::progress(int i_progress) {
    if (m_backend) {
        m_backend->progress(m_id, i_progress);
    }
}

template<class Req, class Rep>
bool ReactorServer<Req,Rep>::Session::isAlive() const {
    return (m_backend && m_backend->isAlive(m_id));
}

template<class Req, class Rep>
void ReactorServer<Req,Rep>::Session::reply(std::unique_ptr<Rep> i_reply) {
    if (nullptr == m_backend) {
        return;
    }
    m_backend->reply(m_id, std::move(i_reply));
    m_backend.reset();
}

template<class Req, class Rep>
void ReactorServer<Req,Rep>::Session::fail() {
    if (nullptr == m_backend) {
        return;
    }
    m_backend->progress(m_id, PROG_FAILED);
    m_backend.reset();
}

template<class Req, class Rep>
ReactorServer<Req, Rep>::ReactorServer(ParticipantPtr i_participant, std::string const& i_service)
: m_backend(std::make_shared<Backend>(i_participant, i_service))
{
}

}
