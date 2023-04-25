#pragma once
#include <memory>

#include "LetsTalk.hpp"
#include "Reactor.h"
#include "Reactor.hpp"
namespace lt {

template <class Req, class Rep, class ProgressData = reactor_void_progress>
class ReactorServer : public std::enable_shared_from_this<ReactorServer<Req, Rep, ProgressData>> {
 public:
  class Session {
   public:
    /// Inspect the request data for this session
    Req const* request() const { return m_request.get(); }

    /// Transmit progress information to the client
    void progress(int i_progress, ProgressData const& i_data);

    typename std::enable_if<std::is_same<ProgressData, reactor_void_progress>::value>::type progress(int i_progress)
    {
      progress(i_progress, reactor_void_progress());
    }

    /// Check that the client hasn't cancelled this session, or that it isn't already complete
    bool isAlive() const;

    /// Send the reply, ending the session
    void reply(Rep const& i_reply);

    /// Mark the request as failed
    void fail();

   protected:
    friend class ReactorServer;

    Session(std::shared_ptr<ReactorServer> i_reactor, Req const& i_request, Guid i_id);

    std::shared_ptr<ReactorServer> m_reactor;
    Req m_request;
    Guid m_id;
  };

  ReactorServer(ParticipantPtr m_participant, std::string const& i_service);
  ~ReactorServer();

  /// Check if there is a pending session
  bool havePendingSession() const;

  /// Get the next pending request as a new session object. If there are no pending requests,
  /// this retuns a session object where isAlive() returns false
  Session getPendingSession(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0));

  /// For debugging. Prints connection status if LT_VERBOSE is defined
  void logConnectionStatus() const;

 protected:
  void recordReply(Guid const& i_id);
  void reply(Guid const& i_id, Rep const& i_reply);
  std::pair<Guid, Req> getPendingSessionData(std::chrono::nanoseconds i_wait);
  void pruneDeadSessions();
  void progress(Guid const& i_id, int i_progress, efr::SerializedPayload_t& i_payload);
  bool isAlive(Guid const& i_id) const;

  enum State { STARTING, RUNNING, CANCELLING, CANCELLED, FAILING, FAILED, SUCCEEDING, SUCCEED };

  ParticipantPtr m_participant;
  Publisher m_replySender;  // is type erased already
  Publisher m_progressSender;
  std::string m_service;

  ////////////////////////////////////////////////////////////
  // The session and pending data are guarded by m_mutex
  using LockGuard = std::unique_lock<std::mutex>;
  mutable std::mutex m_mutex;
  std::condition_variable m_arePending;
  std::map<Guid, State> m_session;
  std::deque<std::pair<Guid, Req>> m_pending;
};

}  // namespace lt
#include "ReactorServerImpl.hpp"