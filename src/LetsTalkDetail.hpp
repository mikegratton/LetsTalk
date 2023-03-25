#pragma once
#include "LetsTalk.hpp"
#include "assert.h"
#include <string>

/**
 * Implementations for class templates are defined here. This has the type-specific 
 * code for pub/sub and req/rep. 
 */

// FastDDS return codes to string translation
namespace eprosima {
namespace fastrtps {
namespace types {
std::ostream& operator<<(std::ostream& os, ReturnCode_t i_return);
}
}
}


namespace lt {

using eprosima::fastrtps::types::ReturnCode_t;

    
/////////////////////////////////////////////////////////////////////////////////
// Inline comparison operators for Guids    
inline bool operator<(Guid const& g1, Guid const& g2) {
    int compare = memcmp(g1.data, g2.data, sizeof(g1.data));
    return (compare < 0 || (compare == 0 && g1.sequence < g2.sequence));    
}

inline bool operator==(Guid const& g1, Guid const& g2) {
    return memcmp(g1.data, g2.data, sizeof(g1.data)) == 0 && g1.sequence == g2.sequence;
}

namespace detail {

// Logging is controlled by this variable, set from the environment on startup
extern const bool LT_VERBOSE;

// The log macro swallows the message is LT_VERBOSE is false
#define LT_LOG if(!::lt::detail::LT_VERBOSE){} else std::cout


////////////////////////////////////////////////////////////////////////////////////////
// TODO does name demangling need to live here?
std::string demangle_name(char const* i_mangled);

template<class T>
std::string get_demangled_name() { return demangle_name(typeid(T).name()); }
////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// GUID_t and SampleId are both converible to the lt::Guid
efr::GUID_t toNative(Guid const& i_guid);
Guid toLib(efr::GUID_t const& i_guid);
efr::SampleIdentity toSampleId(Guid const& i_id);
Guid fromSampleId(efr::SampleIdentity const& i_sampleId);


//////////////////////////////////////////////////////////////////////////////
// ReaderListener class template implementation
// This is in detail because instances should not be visible outside the library
template<class T, class C>
ReaderListener<T,C>::ReaderListener(C i_callback)
    : m_callback(i_callback) {
}


template<class T, class C>
void ReaderListener<T,C>::on_data_available(efd::DataReader* i_reader) {
    std::size_t expected = i_reader->get_unread_count();
    for (std::size_t i=0; i<expected; i++) {
        std::unique_ptr<T> data(new T);
        efd::SampleInfo info;
        auto code = i_reader->take_next_sample(data.get(), &info);
        if (code == ReturnCode_t::RETCODE_OK) {
            if (info.valid_data) {
                m_callback(std::move(data));
            }
        } else {
            LT_LOG << i_reader->get_subscriber()->get_participant()
                   << " " <<  i_reader->get_topicdescription()->get_name()
                   << " callback has an incomplete sample: " << code << "\n";
            break;
        }
    }
}

template<class T, class C>
void ReaderListener<T,C>::on_sample_rejected(efd::DataReader* reader,
        const efd::SampleRejectedStatus& status) {
    logSampleRejected(reader, status);
}

template<class T, class C>
void ReaderListener<T,C>::on_requested_incompatible_qos(efd::DataReader* reader,
        const efd::RequestedIncompatibleQosStatus& status) {
    logIncompatibleQos(reader, status);
}

template<class T, class C>
void ReaderListener<T,C>::on_sample_lost(efd::DataReader* reader,
        const efd::SampleLostStatus& status) {
    logLostSample(reader, status);
}

////////////////////////////////////////////////////////////////////////////////////////
// ReaderWithIdListener implementation. This only differs in on_data_available
template<class T, class C>
void ReaderWithIdListener<T,C>::on_data_available(efd::DataReader* i_reader) 
{
    std::size_t expected = i_reader->get_unread_count();
    for (std::size_t i=0; i<expected; i++) {
        std::unique_ptr<T> data(new T);
        efd::SampleInfo info;
        auto code = i_reader->take_next_sample(data.get(), &info);
        if (code == ReturnCode_t::RETCODE_OK) {
            if (info.valid_data) {
                m_callbackWithId(std::move(data), detail::fromSampleId(info.sample_identity), 
                           detail::fromSampleId(info.related_sample_identity));
            }
        } else {
            LT_LOG << i_reader->get_subscriber()->get_participant()
                   << " " <<  i_reader->get_topicdescription()->get_name()
                   << " callback has an incomplete sample: " << code << "\n";
            break;
        }
    }
}


} // namespace detail

//////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Publish/Subscribe
// Most of these try to push work off on generic doSubscribe/doPublish/doAdvertise methods
template<class T, class C>
void Participant::subscribe(std::string const& i_topic, C i_callback, std::string const& i_qosProfile,
                            int i_historyDepth) {
    auto listener = detail::makeListener<T, C>(i_callback);
    doSubscribe(i_topic, efd::TypeSupport(new PubSubType<T>()), listener, i_qosProfile, i_historyDepth);
}

template<class T, class C>
void Participant::subscribeWithId(std::string const& i_topic, C i_callback, std::string const& i_qosProfile,
                            int i_historyDepth) {
    auto listener = detail::makeListenerWithId<T, C>(i_callback);
    doSubscribe(i_topic, efd::TypeSupport(new PubSubType<T>()), listener, i_qosProfile, i_historyDepth);
}

template<class T>
QueuePtr<T> Participant::subscribe(std::string const& i_topic,
                                   std::string const& i_qosProfile, int i_historyDepth) {
    auto queue = std::make_shared<ThreadSafeQueue<T>>(i_historyDepth);
    auto listener = detail::makeListener<T>([queue](std::unique_ptr<T> i_sample) {
        queue.push(std::move(i_sample));
    }, i_topic, shared_from_this());
    doSubscribe(i_topic, efd::TypeSupport(new PubSubType<T>()), listener, i_qosProfile, 1);
}


template<class T>
Publisher Participant::advertise(std::string const& i_topic, std::string const& i_qosProfile,
                                 int i_historyDepth) {
    return doAdvertise(i_topic, efd::TypeSupport(new PubSubType<T>()), i_qosProfile, i_historyDepth);
}


template<class T>
bool Publisher::publish(std::unique_ptr<T> i_data) {
    return doPublish(i_data.get());
}

template<class T>
bool Publisher::publish(std::unique_ptr<T> i_data, Guid const& i_myId, Guid const& i_relatedId, bool i_bad) {
    return doPublish(i_data.get(), i_myId, i_relatedId, i_bad);
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
// Request/Reply

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
template<class Req, class Rep, class C>
class ServiceProvider : public efd::DataReaderListener, ActiveObject {
protected:
    Publisher m_sender;
    C m_providerCallback;
    Guid m_Id;

public:

    ServiceProvider(C i_providerCallback, Publisher i_publisher)
        : m_sender(i_publisher)
        , m_providerCallback(i_providerCallback)
        , m_Id(m_sender.guid()) {
    }

    void on_data_available(efd::DataReader* i_reader) override {
        if (nullptr == i_reader) {
            return;
        }
        Req* data = new Req();
        efd::SampleInfo info;
        auto code = i_reader->take_next_sample(data, &info);
        if (code == ReturnCode_t::RETCODE_OK) {
            if (info.valid_data) {
                m_Id.increment();
                Guid relatedId = detail::fromSampleId(info.sample_identity);
                submitJob([this, data, relatedId]() {
                    std::unique_ptr<Req> udata(data);
                    std::unique_ptr<Rep> reply;
                    try {
                        reply = m_providerCallback(std::move(udata));
                    } catch (...) {
                        reply = nullptr;                        
                    }
                    bool isBad = false;
                    if (nullptr == reply) {
                        isBad = true;
                        reply = std::unique_ptr<Rep>(new Rep);
                        std::cout << "Setting error bit in reply\n";
                    }
                    m_sender.publish(std::move(reply), m_Id, relatedId, isBad);
                });
            }
        }
    }
};

} // namespace detail


//////////////////////////////////////////////////////////////////////////////////////////
// Replier creation: just save the listener inside the FastDDS data reader object
template<class Req, class Rep, class C>
void Participant::advertise(std::string const& i_serviceName, C i_serviceProvider) {
    Publisher sender = advertise<Rep>(detail::replyName(i_serviceName));
    auto listener = new detail::ServiceProvider<Req, Rep, C>(i_serviceProvider, sender);
    doSubscribe(detail::requestName(i_serviceName), efd::TypeSupport(new PubSubType<Req>()), listener, "", 1);
}

///////////////////////////////////////////////////////////////////////////////////
// Requester definitions
template<class Req, class Rep>
Requester<Req, Rep> Participant::request(std::string const& i_serviceName) {
    return Requester<Req, Rep>(shared_from_this(), i_serviceName);
}

template<class Req, class Rep>
void Requester<Req, Rep>::OnReply::on_data_available(efd::DataReader* i_reader) {
    std::unique_ptr<Rep> data(new Rep);
    efd::SampleInfo info;
    auto code = i_reader->take_next_sample(data.get(), &info);
    if (code == ReturnCode_t::RETCODE_OK) {
        if (info.valid_data) {
            Guid good = detail::fromSampleId(info.related_sample_identity);
            Guid bad = good.makeBadVersion();
            std::unique_lock<std::mutex> guard(requester->m_lock);
            auto it = requester->m_requests.find(good);
            if (it != requester->m_requests.end()) {    
                it->second.set_value(std::move(data));
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


template<class Req, class Rep>
Requester<Req, Rep>::Requester(ParticipantPtr i_participant,
                               std::string const& i_serviceName)
    : m_serviceName(i_serviceName) {
    auto type = efd::TypeSupport(new PubSubType<Req>());
    auto topic = i_participant->getTopic(detail::requestName(i_serviceName), type, 1);
    if (nullptr == topic) {
        m_requestPub = nullptr;
        return;
    }
    auto ddsPublisher = i_participant->getRawPublisher();
    efd::DataWriterQos qos = ddsPublisher->get_default_datawriter_qos();
    efd::DataWriter* rawWriter = ddsPublisher->create_datawriter(topic, qos);
    auto writerDeleter = [ddsPublisher](efd::DataWriter* raw) {
        ddsPublisher->delete_datawriter(raw);
    };
    m_requestPub = std::shared_ptr<efd::DataWriter>(rawWriter, writerDeleter);
    m_sessionId = detail::toLib(rawWriter->guid());
    auto listen = new OnReply(this);
    i_participant->doSubscribe(detail::replyName(i_serviceName),
                               efd::TypeSupport(new PubSubType<Rep>()), listen, "", 1);
}

template<class Req, class Rep>
Requester<Req, Rep>::Requester(Requester const& i_other)
: m_requestPub(i_other.m_requestPub)
, m_sessionId(i_other.m_sessionId)
, m_serviceName(i_other.m_serviceName)
{
}

template<class Req, class Rep>
Requester<Req, Rep>::Requester(Requester&& i_other)
: m_requestPub(i_other.m_requestPub)
, m_sessionId(i_other.m_sessionId)
, m_requests(std::move(i_other.m_requests))
, m_serviceName(i_other.m_serviceName)
{
}

template<class Req, class Rep>
Requester<Req, Rep> const& Requester<Req, Rep>::operator=(Requester const& i_other)
{
    m_requestPub = i_other.m_requestPub;
    m_sessionId = i_other.m_sessionId;
    m_serviceName = i_other.m_serviceName;
}

template<class Req, class Rep>
std::future<std::unique_ptr<Rep>> Requester<Req, Rep>::request(std::shared_ptr<Req> i_request) {
    // Determine the request id
    efr::WriteParams params;
    m_sessionId.increment();
    params.sample_identity(detail::toSampleId(m_sessionId));
    // Send the request
    m_requestPub->write(i_request.get(), params);
    std::unique_lock<std::mutex> guard(m_lock);
    return m_requests[m_sessionId].get_future();
}
} // namespace lt
