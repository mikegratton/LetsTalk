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

namespace lt
{

namespace efd = eprosima::fastdds::dds;
namespace efr = eprosima::fastrtps::rtps;


class Participant;
using ParticipantPtr = std::shared_ptr<Participant>;
class Publisher;


namespace detail
{

/*
 * Get the type name for T using RTTI and (potentially) 
 * a demangle call to the compiler library
 */
template<class T> std::string get_demangled_name();

class SubscriberCounter;

/**
 * ReaderListener calls the installed callback when data is available
 */
template<class T, class C>
class ReaderListener : public efd::DataReaderListener
{
public:
    using type = T;
    
    ReaderListener(C i_callback, std::string const& i_topic, ParticipantPtr i_participant);
    
    void on_data_available(efd::DataReader* i_reader) override;
    
    void on_subscription_matched(efd::DataReader*, 
                                efd::SubscriptionMatchedStatus const& info) override;
                                
    void on_sample_rejected(efd::DataReader* reader, 
                            const efd::SampleRejectedStatus& status) override;
                            
    void on_requested_incompatible_qos(efd::DataReader* reader, 
                                       const efd::RequestedIncompatibleQosStatus& status) override;
                                       
    void on_sample_lost(efd::DataReader* reader, const efd::SampleLostStatus& status) override;

protected:
    C m_callback;
    std::string m_topic;
    ParticipantPtr m_participant;
};

/**
 * Helper function to create listener instances from callbacks
 */
template<class T, class C>
efd::DataReaderListener* makeListener(C i_callback, std::string const& i_topic, ParticipantPtr i_participant)
{
    return new ReaderListener<T,C>(i_callback, i_topic, i_participant);
}


/**
 * SubscriberCounter updates the number of subscribers on a topic as they are discovered
 * or lost.
 */
class SubscriberCounter : public efd::DataWriterListener {
public:
    SubscriberCounter(std::string const& i_topic, ParticipantPtr i_participant);

    void on_publication_matched(efd::DataWriter*, efd::PublicationMatchedStatus const& info) override;
    std::string m_topic;
    ParticipantPtr m_participant;
};

/**
 * ParticipantLogger logs messages about participant discovery in verbose mode
 */
class ParticipantLogger : public efd::DomainParticipantListener {
public:
    /*
    std::function<void(efd::DomainParticipant* i_participant, 
                       efr::ParticipantDiscoveryInfo&& i_info)> m_callback;
                    */
    ParticipantLogger();
    /*
    void on_participant_discovery(efd::DomainParticipant* i_participant, 
                                  efr::ParticipantDiscoveryInfo&& i_info) override;            
    */
};

void logSubscriptionMatched(ParticipantPtr const& i_participant, std::string const& i_topic,
                            efd::SubscriptionMatchedStatus const& info);
void logSampleRejected(ParticipantPtr const& i_participant, std::string const& i_topic,
                            const efd::SampleRejectedStatus& status);
void logIncompatibleQos(ParticipantPtr const& i_participant, std::string const& i_topic,
                            const efd::RequestedIncompatibleQosStatus& status);
void logLostSample(ParticipantPtr const& i_participant, std::string const& i_topic,
                            const efd::SampleLostStatus& status);

} // detail
} // lt
