#pragma once
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <memory>
#include "PubSubType.hpp"
namespace lt
{
class Participant;
using ParticipantPtr = std::shared_ptr<Participant>;

class Publisher {
public:

    operator bool() const { return isOkay(); }
    bool isOkay() const { return !(nullptr == m_writer || nullptr == m_serializer); }

    template<class T>
    bool publish(std::unique_ptr<T> i_data) {
        // TODO type safety check
        return doPublish(i_data.get());        
    }

protected:
    friend Participant;

    Publisher(std::shared_ptr<eprosima::fastdds::dds::TopicDataType> i_serializer,
              std::shared_ptr<eprosima::fastdds::dds::DataWriter> i_writer);
    bool doPublish(void* i_data);
    
    std::shared_ptr<eprosima::fastdds::dds::TopicDataType> m_serializer;    
    std::shared_ptr<eprosima::fastdds::dds::DataWriter> m_writer;
};
    

namespace detail
{
class SubscriberCounter;
    
template<class T, class C>
class ReaderListener : public eprosima::fastdds::dds::DataReaderListener
{
public:
    using type = T;
    
    ReaderListener(C i_callback, std::string const& i_topic, ParticipantPtr i_participant);
    
    void on_data_available(::eprosima::fastdds::dds::DataReader* i_reader) override
    {        
        namespace efd = eprosima::fastdds::dds;
        std::unique_ptr<T> data(new T);
        efd::SampleInfo info;
        if(i_reader->take_next_sample(data.get(), &info) == eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK) {
            if (info.valid_data) {
                m_callback(std::move(data));                
            }
        }
    }
    
    void on_subscription_matched(eprosima::fastdds::dds::DataReader*, 
                                eprosima::fastdds::dds::SubscriptionMatchedStatus const& info) override;
    
protected:
    C m_callback;
    std::string m_topic;
    ParticipantPtr m_participant;
};

template<class T, class C>
ReaderListener<T,C>* makeListener(C i_callback, std::string const& i_topic, ParticipantPtr i_participant)
{
    return new ReaderListener<T,C>(i_callback, i_topic, i_participant);
}

}
}
