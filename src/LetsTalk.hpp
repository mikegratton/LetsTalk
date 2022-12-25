#pragma once
#include <map>
#include <vector>
#include <mutex>
#include "LetsTalkFwd.hpp"
#include "PubSubType.hpp"

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
    void subscribe(std::string const& i_topic, C i_callback, std::string const& i_qosProfile = "", 
                   int i_historyDepth=1) {
        TypeSupport type(new PubSubType<T>);
        auto listener = detail::makeListener<T, C>(i_callback, i_topic, shared_from_this());
        doSubscribe(i_topic, type, listener, i_qosProfile, i_historyDepth);
    }

    void unsubscribe(std::string const& i_topic);

    template<class T>
    Publisher advertise(std::string const& i_topic, std::string const& i_qosProfile = "",
        int i_historyDepth=1 ) {
        TypeSupport type(new PubSubType<T>);
        return doAdvertise(i_topic, type, i_qosProfile, i_historyDepth);
    }

    int publisherCount(std::string const& i_topic) const;

    int subscriberCount(std::string const& i_topic) const;

    std::string topicType(std::string const& i_topic) const;
    
    std::shared_ptr<efd::DomainParticipant> getRawParticipant() { return m_participant; }
    std::shared_ptr<efd::Publisher> getRawPublisher() { return m_publisher; }
    std::shared_ptr<efd::Subscriber> getRawSubscriber() { return m_subscriber; }
    
protected:

    Topic* getTopic(std::string const& i_topic, TypeSupport i_type, int i_historyDepth=1);
    Publisher doAdvertise(std::string const& i_topic, TypeSupport i_type, 
                          std::string const& i_qosProfile, int i_historyDepth);
    void doSubscribe(std::string const& i_topic, TypeSupport i_type,
                     efd::DataReaderListener* i_listener, std::string const& i_qosProfile,
                     int i_historyDepth );
    
    void updatePublisherCount(std::string const& i_topic, int i_update);
    void updateSubscriberCount(std::string const& i_topic, int i_update);
    
    template<class T, class C> friend class detail::ReaderListener;
    friend class Publisher;
    friend class detail::SubscriberCounter;

    std::shared_ptr<efd::DomainParticipant> m_participant;
    std::shared_ptr<efd::Publisher> m_publisher;
    std::shared_ptr<efd::Subscriber> m_subscriber;

    mutable std::mutex m_countMutex; // Guards the pub/sub count maps
    std::map<std::string, int> m_subscriberCount; // Number of readers per topic
    std::map<std::string, int> m_publisherCount; // Number of writers per topic
};

std::ostream& operator<<(std::ostream& os, eprosima::fastrtps::types::ReturnCode_t i_return);

}

#include "LetsTalkDetail.hpp"
