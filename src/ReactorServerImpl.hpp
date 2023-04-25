#include "ReactorServer.hpp"

namespace lt {

template <class Req, class Rep, class ProgressData>
void ReactorServer<Req, Rep, ProgressData>::reply(Guid const& i_id, Rep const& i_reply)
{
  recordReply(i_id);
  m_replySender.publish(i_reply, m_replySender.guid(), i_id);
}

template <class Req, class Rep, class ProgressData>
std::pair<Guid, Req> ReactorServer<Req, Rep, ProgressData>::getPendingSessionData(std::chrono::nanoseconds i_wait)
{
  LockGuard guard(m_mutex);
  if (m_arePending.wait_for(guard, i_wait, [this]() { return !m_pending.empty(); })) {
    auto id = m_pending.front().first;
    auto request = std::move(m_pending.front());
    m_pending.pop_front();
    LT_LOG << m_participant.get() << ":" << m_service << "-server"
           << "  Starting new session ID " << id << "\n";
    return request;
  }
  return {Guid::UNKNOWN(), Req()};
}

template <class Req, class Rep, class P>
typename ReactorServer<Req, Rep, P>::Session ReactorServer<Req, Rep, P>::getPendingSession(
    std::chrono::nanoseconds i_wait)
{
  logConnectionStatus();
  pruneDeadSessions();
  auto s = getPendingSessionData(i_wait);
  if (s.first != Guid::UNKNOWN()) { return Session(this->shared_from_this(), s.second, s.first); }
  return Session(nullptr, Req(), Guid::UNKNOWN());
}

template <class Req, class Rep, class ProgressData>
void ReactorServer<Req, Rep, ProgressData>::Session::progress(int i_progress, ProgressData const& i_data)
{
  if (m_reactor) {
    efr::SerializedPayload_t payload;
    if (typeid(ProgressData) != typeid(reactor_void_progress)) {
      PubSubType<ProgressData> ser;
      ser.serialize(&const_cast<ProgressData&>(i_data), &payload);
    }
    m_reactor->progress(m_id, i_progress, payload);
  }
}

template <class Req, class Rep, class P>
void ReactorServer<Req, Rep, P>::Session::reply(Rep const& i_reply)
{
  if (nullptr == m_reactor) { return; }
  m_reactor->reply(m_id, std::move(i_reply));
  m_reactor.reset();
}

template <class Req, class Rep, class P>
void ReactorServer<Req, Rep, P>::Session::fail()
{
  if (nullptr == m_reactor) { return; }
  progress(PROG_FAILED);
  m_reactor.reset();
}

template <class Req, class Rep, class P>
ReactorServer<Req, Rep, P>::ReactorServer(ParticipantPtr i_participant, std::string const& i_service)
    : m_participant(i_participant),
      m_replySender(m_participant->advertise<Rep>(reactorReplyName(i_service))),
      m_progressSender(i_participant->advertise<reactor_progress>(reactorProgressName(i_service))),
      m_service(i_service)
{
  auto commandCallback = [this](reactor_command const&, Guid const&, Guid const& id) {
    // Need to use related ID because entity codes don't match
    LT_LOG << m_participant.get() << ":" << m_service << "-server"
           << "  Session ID " << id << " cancelled\n";
    LockGuard guard(m_mutex);
    m_session[id] = CANCELLED;
  };
  m_participant->subscribe<reactor_command>(reactorCommandName(i_service), commandCallback);
  auto reqCallback = [this](Req const& i_req, Guid const& id, Guid const& /*relatedId*/) {
    LT_LOG << m_participant.get() << ":" << m_service << "-server"
           << "  Request ID " << id << " enqued as pending\n";
    LockGuard guard(m_mutex);
    m_pending.emplace_back(id, i_req);
  };
  m_participant->subscribe<Req>(reactorRequestName(m_service), reqCallback);
}

template <class Req, class Rep, class P>
ReactorServer<Req, Rep, P>::Session::Session(std::shared_ptr<ReactorServer> i_reactor, Req const& i_request, Guid i_id)
    : m_reactor(i_reactor), m_request(i_request), m_id(i_id)
{
  if (m_reactor) { progress(PROG_START); }
}

template <class Req, class Rep, class ProgressData>
void ReactorServer<Req, Rep, ProgressData>::logConnectionStatus() const
{
  LT_LOG << m_participant.get() << " ";
  LT_LOG << "---- " << m_service << " Server ----\n";
  LT_LOG << "               Request pubs: " << m_participant->publisherCount(reactorRequestName(m_service)) << "\n";
  LT_LOG << "               Command pubs: " << m_participant->publisherCount(reactorCommandName(m_service)) << "\n";
  LT_LOG << "               Reply subs: " << m_participant->subscriberCount(reactorReplyName(m_service)) << "\n";
  LT_LOG << "               Progress subs: " << m_participant->subscriberCount(reactorProgressName(m_service)) << "\n";
  LT_LOG << m_participant.get() << "\n";
}

template <class Req, class Rep, class ProgressData>
void ReactorServer<Req, Rep, ProgressData>::progress(Guid const& i_id, int i_progress,
                                                     efr::SerializedPayload_t& i_payload)
{
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
  reactor_progress message;
  message.progress(i_progress);
  message.data().insert(message.data().end(), i_payload.data, i_payload.data + i_payload.length);
  m_progressSender.publish(message, m_progressSender.guid(), i_id);
}

template <class Req, class Rep, class ProgressData>
bool ReactorServer<Req, Rep, ProgressData>::isAlive(Guid const& i_id) const
{
  LockGuard guard(m_mutex);
  auto it = m_session.find(i_id);
  if (it == m_session.end()) { return false; }
  switch (it->second) {
    case STARTING:
    case RUNNING: return true;
    default: return false;
  }
}

template <class Req, class Rep, class ProgressData>
bool ReactorServer<Req, Rep, ProgressData>::Session::isAlive() const
{
  if (m_reactor) { return m_reactor->isAlive(m_id); }
  return false;
}

template <class Req, class Rep, class ProgressData>
void ReactorServer<Req, Rep, ProgressData>::recordReply(Guid const& i_id)
{
  efr::SerializedPayload_t noPayload;
  progress(i_id, PROG_SUCCESS, noPayload);
  LockGuard guard(m_mutex);
  auto it = m_session.find(i_id);
  if (it != m_session.end()) { it->second = SUCCEED; }
  LT_LOG << m_participant.get() << ":" << m_service << "-server"
         << "  Finish session ID " << i_id << "\n";
}

template <class Req, class Rep, class ProgressData>
void ReactorServer<Req, Rep, ProgressData>::pruneDeadSessions()
{
  LockGuard guard(m_mutex);
  std::vector<Guid> dead;
  for (auto const& p : m_session) {
    switch (p.second) {
      case CANCELLED:
      case FAILED:
      case SUCCEED: dead.push_back(p.first);
      default:;
    }
  }
  for (auto const& id : dead) { m_session.erase(id); }
}

template <class Req, class Rep, class ProgressData>
bool ReactorServer<Req, Rep, ProgressData>::havePendingSession() const
{
  LockGuard guard(m_mutex);
  return m_pending.size() > 0;
}

template <class Req, class Rep, class ProgressData>
ReactorServer<Req, Rep, ProgressData>::~ReactorServer()
{
  LockGuard guard(m_mutex);
  m_participant->unsubscribe(reactorCommandName(m_service));
  m_participant->unsubscribe(reactorRequestName(m_service));
  m_participant->unadvertise(reactorReplyName(m_service));
  m_participant->unadvertise(reactorProgressName(m_service));
}

}  // namespace lt