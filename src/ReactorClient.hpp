#pragma once
#include <future>
#include <mutex>

#include "LetsTalk.hpp"
#include "Reactor.hpp"

namespace lt {

template <class Req, class Rep, class ProgressData = reactor_void_progress>
class ReactorClient : public std::enable_shared_from_this<ReactorClient<Req, Rep, ProgressData>> {
 public:
  /**
   * Construct a reactor client that makes requests to i_serviceName
   * @param i_participant Participant to use for communications
   * @param i_serviceName Name of this service
   */
  ReactorClient(ParticipantPtr i_participant, std::string const& i_serviceName);
  ~ReactorClient();

  /**
   * Each session is modeled by this class. It is lightweight and copyable.
   */
  class Session {
   public:
    /// Get the current progress (0 to 100)
    int progress() const;

    // Get the progress data. All data sent is queued.
    bool progressData(ProgressData& o_data, std::chrono::nanoseconds const& i_wait = std::chrono::nanoseconds(0));

    /// Check if this session is alive
    bool isAlive() const;

    /// Cancel the request
    void cancel();

    /// Get the response. Will return false if the response is not ready by i_wait
    bool get(Rep& o_reply, std::chrono::nanoseconds const& i_wait = std::chrono::nanoseconds(0));

   protected:
    friend class ReactorClient;
    Session(std::shared_ptr<ReactorClient> m_reactor, Guid i_id);

    std::shared_ptr<ReactorClient> m_reactor;
    Guid m_id;
  };

  /// Make a request to the reactor service
  Session request(Req const& i_request);

  /// Logs debug information about the connections if LT_VERBOSE is defined
  void logConnectionStatus() const;

 protected:
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

  void cancel(Guid const& i_id);
  bool get(Rep& o_reply, Guid const& i_id, std::chrono::nanoseconds const& i_wait);
  bool isAlive(Guid const& i_id) const;
  int progress(Guid const& i_id) const;
  bool progressData(ProgressData& o_progress, Guid const& i_id, std::chrono::nanoseconds i_wait);

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
    std::shared_ptr<std::promise<Rep>> promise;
    std::future<Rep> replyFuture;

    SessionData()
        : progress(PROG_SENT), promise(std::make_shared<std::promise<Rep>>()), replyFuture(promise->get_future())
    {
    }
  };

  std::map<Guid, SessionData> m_session;
};

}  // namespace lt

#include "ReactorClientImpl.hpp"