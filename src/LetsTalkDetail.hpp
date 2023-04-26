#pragma once
#include <cassert>
#include <string>

#include "LetsTalk.hpp"
#include "ReaderListenerImpl.hpp"
#include "RequestReplyImpl.hpp"

namespace lt {

using eprosima::fastrtps::types::ReturnCode_t;

//////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Publish/Subscribe
// Most of these try to push work off on generic doSubscribe/doPublish/doAdvertise methods
template <class T, class C>
void Participant::subscribe(std::string const& i_topic, C i_callback, std::string const& i_qosProfile,
                            int i_historyDepth)
{
  auto listener = detail::makeListener<T, C>(i_callback);
  doSubscribe(i_topic, efd::TypeSupport(new PubSubType<T>()), listener, i_qosProfile, i_historyDepth);
}

template <class T>
QueuePtr<T> Participant::subscribe(std::string const& i_topic, std::string const& i_qosProfile, int i_historyDepth)
{
  auto queue = std::make_shared<ThreadSafeQueue<T>>(i_historyDepth);
  auto listener = detail::makeListener<T>([queue](std::unique_ptr<T> i_sample) { queue.push(std::move(i_sample)); },
                                          i_topic, shared_from_this());
  doSubscribe(i_topic, efd::TypeSupport(new PubSubType<T>()), listener, i_qosProfile, 1);
}

template <class T>
Publisher Participant::advertise(std::string const& i_topic, std::string const& i_qosProfile, int i_historyDepth)
{
  return doAdvertise(i_topic, efd::TypeSupport(new PubSubType<T>()), i_qosProfile, i_historyDepth);
}

template <class T>
bool Publisher::publish(T const& i_data)
{
  return doPublish(&const_cast<T&>(i_data));
}

template <class T>
bool Publisher::publish(T const& i_data, Guid const& i_myId, Guid const& i_relatedId, bool i_bad)
{
  return doPublish(&const_cast<T&>(i_data), i_myId, i_relatedId, i_bad);
}

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
// Requester creation
template <class Req, class Rep>
Requester<Req, Rep> Participant::request(std::string const& i_serviceName)
{
  return Requester<Req, Rep>(
      std::make_shared<detail::RequesterImpl<Req, Rep>>(this->shared_from_this(), i_serviceName));
}

}  // namespace lt
