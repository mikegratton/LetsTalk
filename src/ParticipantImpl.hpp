#pragma once
#include <cassert>
#include <string>

#include "LetsTalk.hpp"
#include "ReactorClientImpl.hpp"
#include "ReactorServerImpl.hpp"
#include "ReaderListenerImpl.hpp"
#include "RequestReplyImpl.hpp"

namespace lt {

using eprosima::fastrtps::types::ReturnCode_t;

//////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Publish/Subscribe
// Most of these try to push work off on generic doSubscribe/doPublish/doAdvertise methods

/*
 * Make the listener (which needs the types), then call the type-erased doSubscribe
 */
template <class T, class C>
void Participant::subscribe(std::string const& i_topic, C i_callback, std::string const& i_qosProfile,
                            int i_historyDepth)
{
    auto listener = detail::makeListener<T, C>(i_callback);
    doSubscribe(i_topic, efd::TypeSupport(new detail::PubSubType<T>()), listener, i_qosProfile, i_historyDepth);
}

/*
 * Make the specialized queue listener (which needs the types), then call the type-erased doSubscribe
 */
template <class T>
QueuePtr<T> Participant::subscribe(std::string const& i_topic, std::string const& i_qosProfile, int i_historyDepth)
{
    auto queue = std::make_shared<ThreadSafeQueue<T>>(i_historyDepth);
    auto listener = detail::makeListener<T>([queue](std::unique_ptr<T> i_sample) { queue.push(std::move(i_sample)); },
                                            i_topic, shared_from_this());
    doSubscribe(i_topic, efd::TypeSupport(new detail::PubSubType<T>()), listener, i_qosProfile, 1);
}

/*
 * Make the writer (which needs the types), then call the type-erased doAdvertise
 */
template <class T>
Publisher Participant::advertise(std::string const& i_topic, std::string const& i_qosProfile, int i_historyDepth)
{
    return doAdvertise(i_topic, efd::TypeSupport(new detail::PubSubType<T>()), i_qosProfile, i_historyDepth);
}

/*
 * Type-using publish method on otherwise type-erased publisher
 */
template <class T>
bool Publisher::publish(T const& i_data)
{
    return doPublish(&const_cast<T&>(i_data));
}

/*
 * Type-using publish method on otherwise type-erased publisher. This one includes sample correlation data.
 */
template <class T>
bool Publisher::publish(T const& i_data, Guid const& i_myId, Guid const& i_relatedId, bool i_bad)
{
    return doPublish(&const_cast<T&>(i_data), i_myId, i_relatedId, i_bad);
}

template <class T>
bool Publisher::publish(std::unique_ptr<T> i_data)
{
    return doPublish(i_data.get());
}

template <class T>
bool Publisher::publish(std::unique_ptr<T> i_data, Guid const& i_myId, Guid const& i_relatedId, bool i_bad)
{
    return doPublish(i_data.get(), i_myId, i_relatedId, i_bad);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Replier creation: just save the listener inside the FastDDS data reader object
template <class Req, class Rep, class C>
void Participant::advertise(std::string const& i_serviceName, C i_serviceProvider)
{
    Publisher sender = advertise<Rep>(detail::replyName(i_serviceName));
    auto listener = new detail::ServiceProvider<Req, Rep, C>(i_serviceName, i_serviceProvider, sender);
    doSubscribe(detail::requestName(i_serviceName), efd::TypeSupport(new detail::PubSubType<Req>()), listener, "", 1);
}

///////////////////////////////////////////////////////////////////////////////////
// Requester creation
template <class Req, class Rep>
Requester<Req, Rep> Participant::makeRequester(std::string const& i_serviceName)
{
    return Requester<Req, Rep>(
        std::make_shared<detail::RequesterImpl<Req, Rep>>(this->shared_from_this(), i_serviceName));
}

/////////////////////////////////////////////////////////////////////////////////////
// Reactor creation
template <class Req, class Rep, class P>
ReactorServer<Req, Rep, P> Participant::makeReactorServer(std::string const& i_serviceName)
{
    return ReactorServer<Req, Rep, P>(
        std::make_shared<detail::ReactorServerBackend<Req, Rep, P>>(this->shared_from_this(), i_serviceName));
}

template <class Req, class Rep, class P>
ReactorClient<Req, Rep, P> Participant::makeReactorClient(std::string const& i_serviceName)
{
    return ReactorClient<Req, Rep, P>(
        std::make_shared<detail::ReactorClientBackend<Req, Rep, P>>(this->shared_from_this(), i_serviceName));
}

}  // namespace lt
