#pragma once
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <memory>
#include <map>
#include <mutex>
#include "PubSubType.hpp"
#include "LetsTalkDetail.hpp"

namespace lt {

class Participant;
using ParticipantPtr = std::shared_ptr<Participant>;

class Participant : public std::enable_shared_from_this<Participant> {
    using TypeSupport = eprosima::fastdds::dds::TypeSupport;
    using Topic = eprosima::fastdds::dds::Topic;
public:
    static ParticipantPtr create(int i_domain = 0);
    ~Participant();

    template<class T, class C>
    void subscribe(std::string const& i_topic, C i_callback) {
        TypeSupport type(new PubSubType<T>);
        doSubscribe(i_topic, type,
                    detail::makeListener<T, C>(i_callback, i_topic, shared_from_this()));
    }

    void unsubscribe(std::string const& i_topic);

    template<class T>
    Publisher advertise(std::string const& i_topic) {
        TypeSupport type(new PubSubType<T>);
        return doAdvertise(i_topic, type);
    }

    int publisherCount(std::string const& i_topic) const;

    int subscriberCount(std::string const& i_topic) const;

    std::string topicType(std::string const& i_topic) const;

protected:

    Topic* getTopic(std::string const& i_topic, TypeSupport i_type);
    Publisher doAdvertise(std::string const& i_topic, TypeSupport i_type);
    void doSubscribe(std::string const& i_topic, TypeSupport i_type,
                     eprosima::fastdds::dds::DataReaderListener* i_listener);
    
    void updatePublisherCount(std::string const& i_topic, int i_update);
    void updateSubscriberCount(std::string const& i_topic, int i_update);
    
    template<class T, class C> friend class detail::ReaderListener;
    friend class Publisher;
    friend class detail::SubscriberCounter;

    std::shared_ptr<eprosima::fastdds::dds::DomainParticipant> m_participant;
    std::shared_ptr<eprosima::fastdds::dds::Publisher> m_publisher;
    std::shared_ptr<eprosima::fastdds::dds::Subscriber> m_subscriber;
    std::map<std::string, eprosima::fastdds::dds::DataReader*> m_reader;

    mutable std::mutex m_countMutex;
    std::map<std::string, int> m_subscriberCount;
    std::map<std::string, int> m_publisherCount;
};

namespace detail {
    
template<class T, class C>
ReaderListener<T,C>::ReaderListener(C i_callback, std::string const& i_topic, ParticipantPtr i_participant)
    : m_callback(i_callback)
    , m_topic(i_topic)
    , m_participant(i_participant)
{ }

template<class T, class C>
void ReaderListener<T,C>::on_subscription_matched(eprosima::fastdds::dds::DataReader*, 
                                                 eprosima::fastdds::dds::SubscriptionMatchedStatus const& info) {
    if (m_participant) {
        m_participant->updatePublisherCount(m_topic, info.current_count_change);
    }
}

}
}
