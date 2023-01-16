#pragma once
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include <memory>

#include "ActiveObject.hpp"


namespace lt
{

namespace efd = eprosima::fastdds::dds;
namespace efr = eprosima::fastrtps::rtps;


class Participant;
using ParticipantPtr = std::shared_ptr<Participant>;

class Publisher;

template<class Req, class Rep>
class Requester;    
    
using Guid = efr::GUID_t;

namespace detail
{

/*
 * Get the type name for T using RTTI and (potentially) 
 * a demangle call to the compiler library
 */
template<class T> std::string get_demangled_name();

/**
 * ReaderListener calls the installed callback when data is available
 */
template<class T, class C>
class ReaderListener : public efd::DataReaderListener
{
public:
    using type = T;
    
    ReaderListener(C i_callback);
    
    void on_data_available(efd::DataReader* i_reader) override;
     
    void on_sample_rejected(efd::DataReader* i_reader, 
                            const efd::SampleRejectedStatus& i_status) override;
                            
    void on_requested_incompatible_qos(efd::DataReader* i_reader, 
                                       const efd::RequestedIncompatibleQosStatus& i_status) override;
                                       
    void on_sample_lost(efd::DataReader* i_reader, const efd::SampleLostStatus& i_status) override;

protected:
    C m_callback;    
};

/**
 * Helper function to create listener instances from callbacks
 */
template<class T, class C>
efd::DataReaderListener* makeListener(C i_callback)
{
    return new ReaderListener<T,C>(i_callback);
}


/**
 * ParticipantLogger logs messages about participant discovery in verbose mode
 */
class ParticipantLogger : public efd::DomainParticipantListener {
public:
    
    using Callback = std::function<void(efd::DomainParticipant* i_participant, 
                       efr::ParticipantDiscoveryInfo&& i_info)>;
    
    ParticipantLogger(ParticipantPtr i_participant);
    
    void on_participant_discovery(efd::DomainParticipant* i_participant, 
                                  efr::ParticipantDiscoveryInfo&& i_info) override;   
                                  
    void on_subscription_matched(efd::DataReader* i_reader,
                                 efd::SubscriptionMatchedStatus const& info) override;
    
    void on_publication_matched(efd::DataWriter* i_writer, 
                                efd::PublicationMatchedStatus const& i_info) override;    
protected:
    ParticipantPtr m_participant;    
    Callback m_callback;    
};

void logSampleRejected(efd::DataReader* i_reader, const efd::SampleRejectedStatus& i_status);
void logIncompatibleQos(efd::DataReader* i_reader, const efd::RequestedIncompatibleQosStatus& i_status);
void logLostSample(efd::DataReader* i_reader, const efd::SampleLostStatus& i_status);

} // detail
} // lt
