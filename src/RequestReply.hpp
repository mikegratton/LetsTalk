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
 * A lightweight requester wrapper
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
  std::string const& serviceName() const;

  std::future<Rep> request(Req const& i_request);

  bool isConnected() const;
  bool impostorsExist() const;

 protected:
  friend class Participant;
  Requester(std::shared_ptr<detail::RequesterImpl<Req, Rep>> i_backend) : m_backend(i_backend) {}
  std::shared_ptr<detail::RequesterImpl<Req, Rep>> m_backend;
};
}  // namespace lt