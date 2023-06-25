#pragma once
#include <future>
#include <memory>
#include <string>

#include "Awaitable.hpp"
#include "Guid.hpp"
#include "ParticipantLogger.hpp"

namespace lt {
class Participant;
namespace detail {
template <class Req, class Rep>
class RequesterImpl;
template <class Req, class Rep>
class ReplierImpl;
}  // namespace detail

/**
 * @brief A lightweight requester wrapper
 *
 * A Requester is an object used for making requests to a remote service.
 * Each request returns a future response that can be waited upon for the
 * reply.  These are constructed by the Participant.
 *
 * @throws std::runtime_error if the service indicates an error.
 */
template <class Req, class Rep>
class Requester {
   public:
    /// Retrieve the name of this service
    std::string const& serviceName() const;

    /// Make a request.
    /// @param i_request request data
    /// @return future reply
    std::future<Rep> request(Req const& i_request);

    /// Make a request.
    /// @param i_request request data
    /// @return future reply
    std::future<Rep> request(std::unique_ptr<Req> i_request);

    /// Check that there is at least one publisher to Req
    bool isConnected() const;

    /// Check that there is at most one subscriber to the Req topic
    bool impostorsExist() const;

   protected:
    friend class Participant;
    Requester(std::shared_ptr<detail::RequesterImpl<Req, Rep>> i_backend) : m_backend(i_backend) {}
    std::shared_ptr<detail::RequesterImpl<Req, Rep>> m_backend;  // This class just wraps access to the shared backend
};

template <class Req, class Rep>
class Replier {
   public:
    class Session {
       public:
        /// Get the request data. Only valid if isAlive() is true
        Req const& request() const { return *m_request; }

        /// Send the reply, ending the session
        void reply(Rep const& i_reply);

        /// Send the reply, ending the session
        void reply(std::unique_ptr<Rep> i_reply);

        /// Mark the request as failed
        void fail();

        /// Check if the session is viable
        bool isAlive() { return m_backend != nullptr; }

       protected:
        friend detail::ReplierImpl<Req, Rep>;
        Session(std::shared_ptr<detail::ReplierImpl<Req, Rep>> i_backend, std::unique_ptr<Req> i_request,
                Guid i_related)
            : m_request(std::move(i_request)), m_relatedGuid(i_related), m_backend(i_backend)
        {
        }

        std::unique_ptr<Req> m_request;
        Guid m_relatedGuid;
        std::shared_ptr<detail::ReplierImpl<Req, Rep>> m_backend;
    };

    Session getPendingSession(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0));

    /// Retrieve the name of this service
    std::string const& serviceName() const;

    /// Check that there is at most one subscriber to the Req topic
    bool impostorsExist() const;

    operator AwaitablePtr() { return m_backend; }

   protected:
    friend class Participant;
    Replier(std::shared_ptr<detail::ReplierImpl<Req, Rep>> i_backend) : m_backend(i_backend) {}
    std::shared_ptr<detail::ReplierImpl<Req, Rep>> m_backend;  // This class just wraps access to the shared backend
};

}  // namespace lt