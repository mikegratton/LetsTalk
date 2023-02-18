#pragma once
#include "LetsTalk.hpp"
#include "Reactor.hpp"
#include <mutex>
#include <future>

namespace lt {
template<class Req, class Rep>
class ReactorClient {
protected: struct Backend;
public:

    ReactorClient(ParticipantPtr i_participant, std::string const& i_serviceName);

    class Session {
    public:
        int progress() const {
            return m_backend->progress(m_id);
        }

        bool isAlive() const {
            return m_backend->isAlive(m_id);
        }

        void cancel() {
            m_backend->cancel(m_id);
        }

        std::unique_ptr<Rep> get(std::chrono::nanoseconds const& i_wait = std::chrono::nanoseconds(0)) {
            return m_backend->get(m_id, i_wait);
        }

        Session(std::shared_ptr<Backend> i_backend, Guid i_id)
            : m_backend(i_backend)
            , m_id(i_id) {
        }
        
    protected:
        
        std::shared_ptr<Backend> m_backend;
        Guid m_id;
    };

    Session request(std::unique_ptr<Req> i_request);
    
    void logConnectionStatus() const { m_backend->logConnectionStatus(); }

protected:

    struct Backend {
        
        void logConnectionStatus() const {
            LT_LOG << m_participant.get() << " ";
            LT_LOG << "---- " << m_service << " Client ----\n";
            LT_LOG << "               Request subs: " << m_participant->subscriberCount(reactorRequestName(m_service)) << "\n";
            LT_LOG << "               Command subs: " << m_participant->subscriberCount(reactorCommandName(m_service)) << "\n";
            LT_LOG << "               Reply pubs: " << m_participant->publisherCount(reactorReplyName(m_service)) << "\n";
            LT_LOG << "               Progress pubs: " << m_participant->publisherCount(reactorProgressName(m_service)) << "\n";
            LT_LOG << m_participant.get() << "\n";
        }

        void cancel(Guid const& i_id) {
            LockGuard guard(m_mutex);
            auto command = std::unique_ptr<reactor_command>(new reactor_command);
            command->command(Command::CANCEL);
            m_commandSender.publish(std::move(command), i_id, Guid::UNKNOWN());
            m_session.erase(i_id);
            LT_LOG << m_participant.get() << ":" << m_service << "-client" 
            << "  Cancelled session ID " << i_id << "\n";
        }

        bool isAlive(Guid const& i_id) {
            LockGuard guard(m_mutex);
            auto it = m_session.find(i_id);
            if (it != m_session.end()) {
                return (it->second.progress >= 0 && it->second.progress < 100);
            }
            return false;
        }

        int progress(Guid const& i_id) {
            LockGuard guard(m_mutex);
            auto it = m_session.find(i_id);
            if (it != m_session.end()) {
                return it->second.progress;
            }
            return PROG_UNKNOWN;
        }

        // type-erased return?
        std::unique_ptr<Rep> get(Guid const& i_id, std::chrono::nanoseconds const& i_wait) {
            LockGuard guard(m_mutex);
            auto it = m_session.find(i_id);
            if (it->second.replyFuture.wait_for(i_wait) == std::future_status::ready) {
                auto rep = std::move(it->second.replyFuture.get());
                m_session.erase(it);
                return rep;
            }
            return nullptr;
        }

        Backend(ParticipantPtr i_participant, std::string const& i_serviceName) 
        : m_participant(i_participant)
        , m_requestSender(i_participant->advertise<Req>(reactorRequestName(i_serviceName)))
        , m_commandSender(i_participant->advertise<reactor_command>(reactorCommandName(i_serviceName)))
        , m_service(i_serviceName)
        , m_lastId(m_requestSender.guid())
        {            
            i_participant->subscribeWithId<Rep>(reactorReplyName(i_serviceName),
            [this](std::unique_ptr<Rep> i_reply, Guid /*sampleId*/, Guid i_relatedId) {                
                LockGuard guard(m_mutex);
                auto it = m_session.find(i_relatedId);
                if (it != m_session.end()) {
                    LT_LOG << m_participant.get() << ":" << m_service 
                    << "-client" << "  Finish session ID " << i_relatedId << "\n";
                    it->second.promise->set_value(std::move(i_reply));
                }
            });
            i_participant->subscribeWithId<reactor_progress>(reactorProgressName(i_serviceName),
            [this](std::unique_ptr<reactor_progress> i_progress, Guid /*sampleId*/, Guid i_relatedId) {       
                LockGuard guard(m_mutex);                                
                auto it = m_session.find(i_relatedId);
                if (it != m_session.end()) {
                    LT_LOG << m_participant.get() << ":" << m_service << "-client" 
                    << "  Session ID " << i_relatedId << " progress " << i_progress->progress() << "\n";
                    it->second.progress = i_progress->progress();
                }
            });
        }
        
        ~Backend()
        {
            LockGuard guard(m_mutex);
            m_participant->unadvertise(reactorCommandName(m_service));
            m_participant->unadvertise(reactorRequestName(m_service));
            m_participant->unsubscribe(reactorReplyName(m_service));
            m_participant->unsubscribe(reactorProgressName(m_service));
        }


        Guid startNewSession(std::unique_ptr<Req> i_request)
        {
            LockGuard guard(m_mutex);
            m_lastId.increment();
            Guid id = m_lastId;
            m_requestSender.publish(std::move(i_request), id, Guid::UNKNOWN());
            auto& state = m_session[id];
            state.progress = PROG_SENT;
            state.promise = std::make_shared<std::promise<std::unique_ptr<Rep>>>();
            state.replyFuture = state.promise->get_future();
            LT_LOG << m_participant.get() << ":" << m_service << "-client" 
            << "  Start new session ID " << id << "\n";
            return id;
        }
        
        ParticipantPtr m_participant;
        Publisher m_requestSender;
        Publisher m_commandSender;
        std::string m_service;
        
        mutable std::mutex m_mutex;
        using LockGuard = std::unique_lock<std::mutex>;        
        Guid m_lastId;
        struct SessionData {
            int progress;
            std::future<std::unique_ptr<Rep>> replyFuture;
            std::shared_ptr<std::promise<std::unique_ptr<Rep>>> promise;
        };
        std::map<Guid, SessionData> m_session;
    };

    std::shared_ptr<Backend> m_backend;    
};

template<class Req, class Rep>
ReactorClient<Req, Rep>::ReactorClient(ParticipantPtr i_participant, std::string const& i_serviceName) {    
    m_backend = std::make_shared<Backend>(i_participant, i_serviceName);
}


template<class Req, class Rep>
typename ReactorClient<Req, Rep>::Session ReactorClient<Req, Rep>::request(std::unique_ptr<Req> i_request) {
    logConnectionStatus();
    return Session(m_backend, m_backend->startNewSession(std::move(i_request)));
}

}
