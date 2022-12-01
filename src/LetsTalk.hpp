#pragma once
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <memory>
#include <map>
#include <vector>
#include <mutex>
#include "PubSubType.hpp"
#include "LetsTalkFwd.hpp"

namespace lt {

class Participant;
using ParticipantPtr = std::shared_ptr<Participant>;

class Participant : public std::enable_shared_from_this<Participant> {
    using TypeSupport = eprosima::fastdds::dds::TypeSupport;
    using Topic = eprosima::fastdds::dds::Topic;
public:
    static ParticipantPtr create(int i_domain = 0, std::string const& i_qosProfile = "");
    ~Participant();

    template<class T, class C>
    void subscribe(std::string const& i_topic, C i_callback, std::string const& i_qos = "") {
        TypeSupport type(new PubSubType<T>);
        auto listener = detail::makeListener<T, C>(i_callback, i_topic, shared_from_this());
        doSubscribe(i_topic, type, listener, i_qos);
    }

    void unsubscribe(std::string const& i_topic);

    template<class T>
    Publisher advertise(std::string const& i_topic, std::string const& i_qos = "") {
        TypeSupport type(new PubSubType<T>);
        return doAdvertise(i_topic, type, i_qos);
    }

    int publisherCount(std::string const& i_topic) const;

    int subscriberCount(std::string const& i_topic) const;

    std::string topicType(std::string const& i_topic) const;
    
    std::shared_ptr<eprosima::fastdds::dds::DomainParticipant> getRawParticipant() { return m_participant; }
    std::shared_ptr<eprosima::fastdds::dds::Publisher> getRawPublisher() { return m_publisher; }
    std::shared_ptr<eprosima::fastdds::dds::Subscriber> getRawSubscriber() { return m_subscriber; }
    
protected:

    Topic* getTopic(std::string const& i_topic, TypeSupport i_type);
    Publisher doAdvertise(std::string const& i_topic, TypeSupport i_type, std::string const& i_qosProfile);
    void doSubscribe(std::string const& i_topic, TypeSupport i_type,
                     eprosima::fastdds::dds::DataReaderListener* i_listener, std::string const& i_qosProfile);
    
    void updatePublisherCount(std::string const& i_topic, int i_update);
    void updateSubscriberCount(std::string const& i_topic, int i_update);
    
    template<class T, class C> friend class detail::ReaderListener;
    friend class Publisher;
    friend class detail::SubscriberCounter;

    std::shared_ptr<eprosima::fastdds::dds::DomainParticipant> m_participant;
    std::shared_ptr<eprosima::fastdds::dds::Publisher> m_publisher;
    std::shared_ptr<eprosima::fastdds::dds::Subscriber> m_subscriber;

    mutable std::mutex m_countMutex; // Guards the pub/sub count maps
    std::map<std::string, int> m_subscriberCount; // Number of readers per topic
    std::map<std::string, int> m_publisherCount; // Number of writers per topic
};

}

#include "LetsTalkDetail.hpp"
