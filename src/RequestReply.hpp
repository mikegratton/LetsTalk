#pragma once
#include <future>
#include <memory>
#include <string>

#include "ParticipantLogger.hpp"

namespace lt {
class Participant;
namespace detail {
template <class Req, class Rep>
class RequesterImpl;
}

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

    /// Check that there is at least one publisher to Req
    bool isConnected() const;

    /// Check that there is at most one subscriber to the Req topic
    bool impostorsExist() const;

   protected:
    friend class Participant;
    Requester(std::shared_ptr<detail::RequesterImpl<Req, Rep>> i_backend) : m_backend(i_backend) {}
    std::shared_ptr<detail::RequesterImpl<Req, Rep>> m_backend;  // This class just wraps access to the shared backend
};
}  // namespace lt