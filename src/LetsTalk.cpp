#include "LetsTalk.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace lt {

namespace efd = eprosima::fastdds::dds;

namespace detail {

#define LT_LOG if(!LT_VERBOSE){} else std::cout

std::string name(ParticipantPtr const& p) {
    if (p) {
        std::stringstream ss;
        ss << p.get();
        return ss.str();
    }
    return "UNKNOWN";
}

class SubscriberCounter : public efd::DataWriterListener {
public:
    SubscriberCounter(std::string const& i_topic, ParticipantPtr i_participant)
        : m_topic(i_topic)
        , m_participant(i_participant) { }

    void on_publication_matched(efd::DataWriter*, efd::PublicationMatchedStatus const& info) override {
        if (m_participant) {
            m_participant->updateSubscriberCount(m_topic, info.current_count_change);
        }
        if (info.current_count_change > 0) {
            LT_LOG << name(m_participant) << " topic \"" << m_topic << "\" matched " << info.current_count << " subscriber(s)\n";
        } else {
            LT_LOG << name(m_participant) << " topic \"" << m_topic << "\" lost " << info.current_count_change << " subscriber(s)\n";
        }
    }

    std::string m_topic;
    ParticipantPtr m_participant;

};

class ParticipantLogger : public efd::DomainParticipantListener {
public:
    std::function<void(efd::DomainParticipant* i_participant, eprosima::fastrtps::rtps::ParticipantDiscoveryInfo&& i_info)> m_callback;
    ParticipantLogger()
        : m_callback([](efd::DomainParticipant*, eprosima::fastrtps::rtps::ParticipantDiscoveryInfo &&) {})
    { }
    void on_participant_discovery(efd::DomainParticipant* i_participant, eprosima::fastrtps::rtps::ParticipantDiscoveryInfo&& i_info) {
        if (i_info.status == eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT) {
            LT_LOG << i_participant << " discovered participant \"" << i_info.info.m_participantName << "\"\n";
            m_callback(i_participant, std::move(i_info));
        } else if (i_info.status == eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::CHANGED_QOS_PARTICIPANT) {
            LT_LOG << i_participant << " saw participant \"" << i_info.info.m_participantName << "\" change qos\n";
        } else {
            LT_LOG << i_participant << " lost participant \"" << i_info.info.m_participantName << "\"\n";
        }
    }
};

void logSubscriptionMatched(ParticipantPtr const& i_participant, std::string const& i_topic,
                            eprosima::fastdds::dds::SubscriptionMatchedStatus const& i_info) {
    if (i_info.current_count_change > 0) {
        LT_LOG << name(i_participant) << " topic \"" << i_topic << "\" matched " << i_info.current_count << " publisher(s)\n";
    } else {
        LT_LOG << name(i_participant) << " topic \"" << i_topic << "\" lost " << -i_info.current_count_change << " publisher(s)\n";
    }
}

bool check_verbose() {
    char* verb = getenv("LT_VERBOSE");
    if (verb && atoi(verb)) {
        return true;
    }
    return false;
}
}


const bool LT_VERBOSE = detail::check_verbose();


ParticipantPtr Participant::create(int i_domain, std::string const& i_qosProfile) {
    auto p = std::make_shared<Participant>();
    auto factory = efd::DomainParticipantFactory::get_instance();
    auto participantDeleter = [factory](efd::DomainParticipant* raw) {
        raw->delete_contained_entities();
        factory->delete_participant(raw);
    };
    efd::DomainParticipant* rawParticipant;
    if (i_qosProfile.empty()) {
        rawParticipant = factory->create_participant(i_domain, factory->get_default_participant_qos(),
                         new detail::ParticipantLogger);
    } else {
        rawParticipant = factory->create_participant_with_profile(i_domain, i_qosProfile,
                         new detail::ParticipantLogger);
    }
    auto part = std::shared_ptr<efd::DomainParticipant>(rawParticipant, participantDeleter);
    p->m_participant = part;
    auto pubDeleter = [part](efd::Publisher* pub) { part->delete_publisher(pub); };
    auto rawPub = p->m_participant->create_publisher(p->m_participant->get_default_publisher_qos());
    p->m_publisher = std::shared_ptr<efd::Publisher>(rawPub, pubDeleter);

    auto subDeleter = [part](efd::Subscriber* sub) { part->delete_subscriber(sub); };
    auto rawSub = p->m_participant->create_subscriber(p->m_participant->get_default_subscriber_qos());
    p->m_subscriber = std::shared_ptr<efd::Subscriber>(rawSub, subDeleter);

    return p;
}

Participant::~Participant() {
    // TODO
    // Unwind topics
}

Publisher Participant::doAdvertise(std::string const& i_topic, efd::TypeSupport i_type,
                                   std::string const& i_qosProfile) {
    auto topic = getTopic(i_topic, i_type);
    if (nullptr == topic) {
        return Publisher(nullptr, nullptr);
    }
    efd::DataWriter* rawWriter;
    if (i_qosProfile.empty()) {
        rawWriter = m_publisher->create_datawriter(topic, m_publisher->get_default_datawriter_qos());
    } else {
        rawWriter = m_publisher->create_datawriter_with_profile(topic, i_qosProfile);
    }
    auto writerDeleter = [this](efd::DataWriter* raw) {
        m_publisher->delete_datawriter(raw);
    };
    auto writer = std::shared_ptr<efd::DataWriter>(rawWriter, writerDeleter);
    writer->set_listener(new detail::SubscriberCounter(i_topic, shared_from_this()));
    LT_LOG << m_participant << " created new publisher for type \"" << i_type->getName() << "\" on topic \"" << i_topic <<"\"\n";
    return Publisher(i_type, writer);
}

void Participant::doSubscribe(std::string const& i_topic, efd::TypeSupport i_type,
                              efd::DataReaderListener* i_listener, std::string const& i_qosProfile) {
    auto reader = m_subscriber->lookup_datareader(i_topic);
    if (reader) {
        m_subscriber->delete_datareader(reader);
    }

    auto topic = getTopic(i_topic, i_type);
    if (i_qosProfile.empty()) {
        reader = m_subscriber->create_datareader(topic, m_subscriber->get_default_datareader_qos());
    } else {
        reader = m_subscriber->create_datareader_with_profile(topic, i_qosProfile);
    }
    reader->set_listener(i_listener);
    LT_LOG << m_participant << " created new subscriber for type \"" << i_type->getName() << "\" on topic \"" << i_topic <<"\"\n";
}

void Participant::unsubscribe(std::string const& i_topic) {
    auto reader = m_subscriber->lookup_datareader(i_topic);
    if (reader) {
        m_subscriber->delete_datareader(reader);
    }
}

void Participant::updatePublisherCount(std::string const& i_topic, int i_update) {
    std::unique_lock<std::mutex> guard(m_countMutex);
    auto it = m_publisherCount.find(i_topic);
    if (it == m_publisherCount.end()) {
        m_publisherCount[i_topic] = i_update;
    } else {
        it->second += i_update;
    }
}

void Participant::updateSubscriberCount(std::string const& i_topic, int i_update) {
    std::unique_lock<std::mutex> guard(m_countMutex);
    auto it = m_subscriberCount.find(i_topic);
    if (it == m_subscriberCount.end()) {
        m_subscriberCount[i_topic] = i_update;
    } else {
        it->second += i_update;
    }
}

int Participant::publisherCount(std::string const& i_topic) const {
    std::unique_lock<std::mutex> guard(m_countMutex);
    auto it = m_publisherCount.find(i_topic);
    if (it == m_publisherCount.end()) {
        return 0;
    } else {
        return it->second;
    }
}

int Participant::subscriberCount(std::string const& i_topic) const {
    std::unique_lock<std::mutex> guard(m_countMutex);
    auto it = m_subscriberCount.find(i_topic);
    if (it == m_subscriberCount.end()) {
        return 0;
    } else {
        return it->second;
    }
}

std::string Participant::topicType(std::string const& i_topic) const {
    auto topic = m_participant->find_topic(i_topic, eprosima::fastrtps::Duration_t(0,10000));
    if (nullptr == topic) {
        return std::string();
    }
    return topic->get_type_name();
}

efd::Topic* Participant::getTopic(std::string const& i_topic, efd::TypeSupport i_type) {
    auto knownType = m_participant->find_type(i_type->getName());
    if (nullptr == knownType) {
        m_participant->register_type(i_type);
        LT_LOG << m_participant << " registered a new type \"" << i_type->getName() << "\"\n";
    }
    auto topic = m_participant->find_topic(i_topic, eprosima::fastrtps::Duration_t(0,10000));
    if (nullptr == topic) {
        topic = m_participant->create_topic(i_topic, i_type->getName(),
                                            m_participant->get_default_topic_qos());
        LT_LOG << m_participant << " started a new topic for type \"" << i_type->getName() << "\" named \"" << i_topic <<"\"\n";

    }
    if (nullptr == topic || topic->get_type_name() != i_type->getName()) {
        return nullptr;
    }
    return topic;
}

Publisher::Publisher(std::shared_ptr<efd::TopicDataType> i_serializer,
                     std::shared_ptr<eprosima::fastdds::dds::DataWriter> i_writer)
    : m_serializer(i_serializer)
    , m_writer(i_writer)
{ }


bool Publisher::doPublish(void* i_data) {
    if (!isOkay() || nullptr == i_data) {
        return false;
    }

    return m_writer->write(i_data);
}

}
