#include "LetsTalk.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>

namespace lt {

namespace efd = eprosima::fastdds::dds;

namespace detail {
class SubscriberCounter : public efd::DataWriterListener {
public:
    SubscriberCounter(std::string const& i_topic, ParticipantPtr i_participant)
        : m_topic(i_topic)
        , m_participant(i_participant) { }

    void on_publication_matched(efd::DataWriter*, efd::PublicationMatchedStatus const& info) override {
        if (m_participant) {
            m_participant->updateSubscriberCount(m_topic, info.current_count_change);
        }
    }

    std::string m_topic;
    ParticipantPtr m_participant;

};

}

ParticipantPtr Participant::create(int i_domain) {
    auto p = std::make_shared<Participant>();
    auto factory = efd::DomainParticipantFactory::get_instance();
    auto participantDeleter = [factory](efd::DomainParticipant* raw) {
        raw->delete_contained_entities();
        factory->delete_participant(raw);
    };
    auto rawParticipant = factory->create_participant(i_domain, factory->get_default_participant_qos());
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

Publisher Participant::doAdvertise(std::string const& i_topic, efd::TypeSupport i_type) {
    auto topic = getTopic(i_topic, i_type);
    if (nullptr == topic) {
        return Publisher(nullptr, nullptr);
    }
    auto rawWriter = m_publisher->create_datawriter(topic, m_publisher->get_default_datawriter_qos());
    auto writerDeleter = [this](efd::DataWriter* raw) {
        m_publisher->delete_datawriter(raw);
    };
    auto writer = std::shared_ptr<efd::DataWriter>(rawWriter, writerDeleter);
    writer->set_listener(new detail::SubscriberCounter(i_topic, shared_from_this()));
    return Publisher(i_type, writer);
}

void Participant::doSubscribe(std::string const& i_topic, efd::TypeSupport i_type,
                              efd::DataReaderListener* i_listener) {
    auto it = m_reader.find(i_topic);
    if (it != m_reader.end()) {
        unsubscribe(i_topic);
    }
    auto topic = getTopic(i_topic, i_type);
    auto* reader = m_subscriber->create_datareader(topic, m_subscriber->get_default_datareader_qos());
    reader->set_listener(i_listener);
    m_reader[i_topic] = reader;
}

void Participant::unsubscribe(std::string const& i_topic) {
    auto it = m_reader.find(i_topic);
    if (it != m_reader.end()) {
        m_subscriber->delete_datareader(it->second);
        m_reader.erase(it);
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
    }
    auto topic = m_participant->find_topic(i_topic, eprosima::fastrtps::Duration_t(0,10000));
    if (nullptr == topic) {
        topic = m_participant->create_topic(i_topic, i_type->getName(),
                                            m_participant->get_default_topic_qos());
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
