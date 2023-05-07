#pragma once
#include <fastdds/dds/publisher/Publisher.hpp>
#include <functional>

#include "LetsTalk.hpp"
#include "LetsTalkFwd.hpp"
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
    /// Ctor. Note this starts the work thread.
    ServiceProvider(C i_providerCallback, Publisher i_publisher)
        : m_sender(i_publisher), m_providerCallback(i_providerCallback), m_Id(m_sender.guid())
    {
    }

    /// Callback for Req
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
                LT_LOG << "Request " << relatedId << " received\n";
                submitJob([this, data, relatedId]() {
                    Rep reply;
                    bool isBad = false;
                    try {
                        reply = m_providerCallback(data);
                    } catch (...) {
                        isBad = true;
                    }
                    LT_LOG << "Sending reply for " << relatedId << ", result okay=" << !isBad << "\n";
                    m_sender.publish(reply, m_Id, relatedId, isBad);
                });
            }
        }
    }
};

/**
 * Backend for requester.
 */
template <class Req, class Rep>
class RequesterImpl {
   public:
    /// Ctor. Set up req and rep subscriptions.
    RequesterImpl(std::shared_ptr<Participant> i_participant, std::string const& i_serviceName)
        : m_participant(i_participant),
          m_serviceName(i_serviceName),
          m_requestPub(m_participant->advertise<Req>(detail::requestName(m_serviceName))),
          m_sessionId(m_requestPub.guid())
    {
        m_participant->subscribe<Rep>(
            detail::replyName(serviceName()),
            [this](Rep const& data, Guid const& nope, Guid const& id) { this->onReply(data, nope, id); });
    }

    /// Stop subscribing to req
    ~RequesterImpl() { m_participant->unsubscribe(detail::replyName(serviceName())); }

    /// Obtain the service name
    std::string const& serviceName() const { return m_serviceName; }

    /// Make a new request
    /// @param i_request Request data
    /// @return future Rep supplied when request fulfilled
    std::future<Rep> request(Req const& i_request)
    {
        // Send the request
        Guid requestId;
        std::future<Rep> future;
        {
            std::unique_lock<std::mutex> guard(m_lock);
            requestId = m_sessionId.increment();
            future = m_requests[requestId].get_future();
        }
        m_requestPub.publish(i_request, requestId, Guid::UNKNOWN());
        LT_LOG << serviceName() << ": Making request " << requestId << "\n";
        return future;
    }

    bool isConnected() const
    {
        int providerCount = m_participant->subscriberCount(detail::requestName(serviceName()));
        if (providerCount > 1) {
            LT_LOG << "Warning! Service " << serviceName() << " has " << providerCount << " providers.";
        }
        return providerCount > 0;
    }

    bool impostorsExist() const { return m_participant->subscriberCount(detail::requestName(serviceName())) > 1; }

   protected:
    /// Callback. Use Guid to check that this reply is for us.
    void onReply(Rep const& data, Guid const& id, Guid const& relatedId)
    {
        LT_LOG << serviceName() << ": Received reply " << relatedId << "\n";

        Guid badId = relatedId.makeBadVersion();
        std::unique_lock<std::mutex> guard(m_lock);
        auto it = m_requests.find(relatedId);
        if (it != m_requests.end()) {
            LT_LOG << serviceName() << ": Recognized as a pending request\n";
            it->second.set_value(data);
            m_requests.erase(it);
            return;
        }
        it = m_requests.find(badId);
        if (it != m_requests.end()) {
            LT_LOG << serviceName() << ": Recognized as a FAILED pending request\n";
            it->second.set_exception(std::make_exception_ptr(std::runtime_error("RPC failed")));
            m_requests.erase(it);
            return;
        }
        LT_LOG << serviceName() << ": Reply " << relatedId << " ignored.\n";
    }

    std::shared_ptr<Participant> m_participant;  //! Related participant
    std::string m_serviceName;                   //! The name of this service
    Publisher m_requestPub;                      //! Publisher for results
    Guid m_sessionId;                            //! Current ID of session in progress

    using Promise = std::promise<Rep>;   //! To be filled when reply arrives
    std::mutex m_lock;                   //! Guards the requests map
    std::map<Guid, Promise> m_requests;  //! All pending requests
};

}  // namespace detail

template <class Req, class Rep>
std::string const& Requester<Req, Rep>::serviceName() const
{
    return m_backend->serviceName();
}

template <class Req, class Rep>
std::future<Rep> Requester<Req, Rep>::request(Req const& i_request)
{
    return m_backend->request(i_request);
}

template <class Req, class Rep>
bool Requester<Req, Rep>::isConnected() const
{
    return m_backend->isConnected();
}

template <class Req, class Rep>
bool Requester<Req, Rep>::impostorsExist() const
{
    return m_backend->impostorsExist();
}

}  // namespace lt
