#pragma once
#include <fastdds/dds/publisher/Publisher.hpp>

#include "LetsTalk.hpp"
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
// Request/Reply
namespace lt {
namespace detail {

//////////////////////////////////////////////////////////
// Request/Reply naming and constants
std::string requestName(std::string i_name);
std::string replyName(std::string i_name);
efr::Time_t getBadTime();
efr::SampleIdentity getBadId();

///////////////////////////////////////////////////////////////////
// ServiceProvider is an internal active object used to convert a
// subscription to a callback to the actual service provision.
// It tracks the Guid of the request automatically.
template <class Req, class Rep, class C>
class ServiceProvider : public efd::DataReaderListener, ActiveObject {
 protected:
  Publisher m_sender;
  C m_providerCallback;
  Guid m_Id;

 public:
  ServiceProvider(C i_providerCallback, Publisher i_publisher)
      : m_sender(i_publisher), m_providerCallback(i_providerCallback), m_Id(m_sender.guid())
  {
  }

  void on_data_available(efd::DataReader* i_reader) override
  {
    if (nullptr == i_reader) { return; }
    Req data;
    efd::SampleInfo info;
    auto code = i_reader->take_next_sample(&data, &info);
    if (code == ReturnCode_t::RETCODE_OK) {
      if (info.valid_data) {
        m_Id.increment();
        Guid relatedId = toLetsTalkGuid(info.sample_identity);
        submitJob([this, data, relatedId]() {
          Rep reply;
          bool isBad = false;
          try {
            reply = m_providerCallback(data);
          } catch (...) {
            isBad = true;
          }
          m_sender.publish(reply, m_Id, relatedId, isBad);
        });
      }
    }
  }
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////////////////////////
// Replier creation: just save the listener inside the FastDDS data reader object
template <class Req, class Rep, class C>
void Participant::advertise(std::string const& i_serviceName, C i_serviceProvider)
{
  Publisher sender = advertise<Rep>(detail::replyName(i_serviceName));
  auto listener = new detail::ServiceProvider<Req, Rep, C>(i_serviceProvider, sender);
  doSubscribe(detail::requestName(i_serviceName), efd::TypeSupport(new PubSubType<Req>()), listener, "", 1);
}

///////////////////////////////////////////////////////////////////////////////////
// Requester definitions
template <class Req, class Rep>
Requester<Req, Rep> Participant::request(std::string const& i_serviceName)
{
  return Requester<Req, Rep>(this->shared_from_this(), i_serviceName);
}

template <class Req, class Rep>
void Requester<Req, Rep>::OnReply::on_data_available(efd::DataReader* i_reader)
{
  Rep data;
  efd::SampleInfo info;
  auto code = i_reader->take_next_sample(&data, &info);
  if (code == ReturnCode_t::RETCODE_OK) {
    if (info.valid_data) {
      Guid good = toLetsTalkGuid(info.related_sample_identity);
      Guid bad = good.makeBadVersion();
      std::unique_lock<std::mutex> guard(requester->m_lock);
      auto it = requester->m_requests.find(good);
      if (it != requester->m_requests.end()) {
        it->second.set_value(data);
        requester->m_requests.erase(it);
        return;
      }
      it = requester->m_requests.find(bad);
      if (it != requester->m_requests.end()) {
        it->second.set_exception(std::make_exception_ptr(std::runtime_error("RPC failed")));
        requester->m_requests.erase(it);
      }
    }
  }
}

template <class Req, class Rep>
Requester<Req, Rep>::Requester(ParticipantPtr i_participant, std::string const& i_serviceName)
    : m_participant(i_participant), m_serviceName(i_serviceName)
{
  auto type = efd::TypeSupport(new PubSubType<Req>());
  auto topic = i_participant->getTopic(detail::requestName(i_serviceName), type, 1);
  if (nullptr == topic) {
    m_requestPub = nullptr;
    return;
  }
  auto ddsPublisher = i_participant->getRawPublisher();
  efd::DataWriterQos qos = ddsPublisher->get_default_datawriter_qos();
  efd::DataWriter* rawWriter = ddsPublisher->create_datawriter(topic, qos);
  auto writerDeleter = [ddsPublisher](efd::DataWriter* raw) { ddsPublisher->delete_datawriter(raw); };
  m_requestPub = std::shared_ptr<efd::DataWriter>(rawWriter, writerDeleter);
  m_sessionId = toLetsTalkGuid(rawWriter->guid());
  auto listen = new OnReply(this);
  i_participant->doSubscribe(detail::replyName(i_serviceName), efd::TypeSupport(new PubSubType<Rep>()), listen, "", 1);
}

template <class Req, class Rep>
bool Requester<Req, Rep>::isConnected() const
{
  int providerCount = m_participant->subscriberCount(detail::requestName(serviceName()));
  if (providerCount > 1) {
    LT_LOG << "Warning! Service " << serviceName() << " has " << providerCount << " providers.";
  }
  return providerCount > 0;
}

template <class Req, class Rep>
bool Requester<Req, Rep>::impostorsExist() const
{
  return m_participant->subscriberCount(detail::requestName(serviceName())) > 1;
}

template <class Req, class Rep>
Requester<Req, Rep>::Requester(Requester const& i_other)
    : m_requestPub(i_other.m_requestPub), m_sessionId(i_other.m_sessionId), m_serviceName(i_other.m_serviceName)
{
}

template <class Req, class Rep>
Requester<Req, Rep>::Requester(Requester&& i_other)
    : m_requestPub(i_other.m_requestPub),
      m_sessionId(i_other.m_sessionId),
      m_requests(std::move(i_other.m_requests)),
      m_serviceName(i_other.m_serviceName)
{
}

template <class Req, class Rep>
Requester<Req, Rep> const& Requester<Req, Rep>::operator=(Requester const& i_other)
{
  m_requestPub = i_other.m_requestPub;
  m_sessionId = i_other.m_sessionId;
  m_serviceName = i_other.m_serviceName;
}

template <class Req, class Rep>
std::future<Rep> Requester<Req, Rep>::request(Req const& i_request)
{
  // Determine the request id
  efr::WriteParams params;
  m_sessionId.increment();
  params.sample_identity(toSampleId(m_sessionId));
  // Send the request
  m_requestPub->write(&const_cast<Req&>(i_request), params);
  std::unique_lock<std::mutex> guard(m_lock);
  return m_requests[m_sessionId].get_future();
}

}  // namespace lt
