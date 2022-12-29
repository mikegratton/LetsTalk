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
namespace efr = eprosima::fastrtps::rtps;

ParticipantPtr Participant::create(int i_domain, std::string const& i_qosProfile) {
    auto p = std::make_shared<Participant>();
    auto factory = efd::DomainParticipantFactory::get_instance();
    
    char const* profileXml = nullptr;
    profileXml = getenv("LT_PROFILE");
    if(profileXml) {
        auto code = factory->load_XML_profiles_file(profileXml);
        LT_LOG << p << ": xml loaded with code " << code;
    } else {
        std::string defaultXml = detail::getDefaultProfileXml();
        factory->load_XML_profiles_string(defaultXml.c_str(), defaultXml.size());
    }
    
    auto participantDeleter = [factory](efd::DomainParticipant* raw) {
        raw->delete_contained_entities();
        factory->delete_participant(raw);
    };
    
    // By default, a listener on the participant intercepts ALL data callbacks.
    // Let's disable that
    efd::StatusMask dontBlockData = efd::StatusMask::all();
    dontBlockData >> efd::StatusMask::data_available() >> efd::StatusMask::data_on_readers();
    
    efd::DomainParticipant* rawParticipant;
    if (i_qosProfile.empty()) {        

        rawParticipant = factory->create_participant(i_domain, 
                                                     factory->get_default_participant_qos(), 
                                                     new detail::ParticipantLogger(), 
                                                     dontBlockData);        
    } else {
        rawParticipant = factory->create_participant_with_profile(i_domain, i_qosProfile,
                         new detail::ParticipantLogger(), dontBlockData);
    }
    
    auto participant = std::shared_ptr<efd::DomainParticipant>(rawParticipant, participantDeleter);
    p->m_participant = participant;
    auto pubDeleter = [participant](efd::Publisher* pub) { participant->delete_publisher(pub); };
    auto rawPub = p->m_participant->create_publisher(p->m_participant->get_default_publisher_qos());
    p->m_publisher = std::shared_ptr<efd::Publisher>(rawPub, pubDeleter);

    auto subDeleter = [participant](efd::Subscriber* sub) { participant->delete_subscriber(sub); };
    auto rawSub = p->m_participant->create_subscriber(p->m_participant->get_default_subscriber_qos());    
    p->m_subscriber = std::shared_ptr<efd::Subscriber>(rawSub, subDeleter);

    return p;
}

Participant::~Participant() {
    m_publisher->delete_contained_entities();        
    m_subscriber->delete_contained_entities();    
    m_participant->delete_contained_entities();
}

Publisher Participant::doAdvertise(std::string const& i_topic, TypeSupport const& i_type,
                                   std::string const& i_qosProfile, int i_historyDepth) {
    auto topic = getTopic(i_topic, i_type, i_historyDepth);
    if (nullptr == topic) {
        return Publisher(nullptr);
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
    auto code = writer->set_listener(new detail::SubscriberCounter(i_topic, shared_from_this()));
    LT_LOG << m_participant << " created new publisher for type \"" << i_type.get_type_name() << "\" on topic \"" << i_topic 
        <<"\"; " << code << "\n";
    return Publisher(writer);
}

void Participant::doSubscribe(std::string const& i_topic, TypeSupport const& i_type,
                              efd::DataReaderListener* i_listener, std::string const& i_qosProfile,
                              int i_historyDepth) {    
    auto reader = m_subscriber->lookup_datareader(i_topic);
    if (reader && reader->type().get_type_name() != i_type.get_type_name()) {
        auto code = m_subscriber->delete_datareader(reader);
        LT_LOG << "Deleted old datareader on " << i_topic << "; " << code << "\n";
    }
    auto topic = getTopic(i_topic, i_type, i_historyDepth);
    if (i_qosProfile.empty()) {
        reader = m_subscriber->create_datareader(topic, m_subscriber->get_default_datareader_qos(), i_listener);
    } else {
        reader = m_subscriber->create_datareader_with_profile(topic, i_qosProfile, i_listener);
    } 
    LT_LOG << m_participant << " created new subscriber for type \"" << i_type->getName() << "\" on topic \"" << i_topic <<"\"\n";
}

void Participant::unsubscribe(std::string const& i_topic) {
    auto reader = m_subscriber->lookup_datareader(i_topic);
    if (reader) {
        auto code = m_subscriber->delete_datareader(reader);
        LT_LOG << "Unsubscribed from " << i_topic << "; " << code << "\n";
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

void Participant::registerType(efd::TypeSupport const& i_type)
{
    i_type.register_type(m_participant.get());    
}

std::string Participant::topicType(std::string const& i_topic) const {
    auto topic = m_participant->find_topic(i_topic, eprosima::fastrtps::Duration_t(0,10000));
    if (nullptr == topic) {
        return std::string();
    }
    return topic->get_type_name();
}

efd::Topic* Participant::getTopic(std::string const& i_topic, efd::TypeSupport const& i_type,
                                  int i_historyDepth) {
        
    registerType(i_type);
    auto topic = m_participant->find_topic(i_topic, eprosima::fastrtps::Duration_t(0,10000));
    if (nullptr == topic) {
        topic = m_participant->create_topic(i_topic, i_type.get_type_name(),
                                            m_participant->get_default_topic_qos());        
        LT_LOG << m_participant << " started a new topic for type " << i_type.get_type_name() 
               << " named \"" << i_topic <<"\"\n";

    }
    if (nullptr == topic) {
        LT_LOG << m_participant << " could not create topic \"" << i_topic << "\"\n";
        return nullptr;
    } else if (topic->get_type_name() != i_type.get_type_name()) {        
        LT_LOG << m_participant << " already has a topic \"" << i_topic << "\", but for type " 
            << topic->get_type_name() << " not the requested type " << i_type.get_type_name() << "\n";        
        return nullptr;
    } else {
        // Set history depth 
        auto qos = topic->get_qos();
        efd::HistoryQosPolicy& history = qos.history();
        history.kind = efd::KEEP_LAST_HISTORY_QOS;
        history.depth = i_historyDepth;
        auto code = topic->set_qos(qos);
        LT_LOG << m_participant << " set history depth to " << i_historyDepth << " on " << i_topic << "; " << code << "\n";
        return topic;
    }
}

Publisher::Publisher(std::shared_ptr<efd::DataWriter> i_writer)
    : m_writer(i_writer)
{ }


bool Publisher::doPublish(void* i_data) {    
    if (!isOkay() || nullptr == i_data) {
        LT_LOG << m_writer << " could not publish sample " << i_data << "\n";
        return false;
    }        
    return m_writer->write(i_data);
}

}
