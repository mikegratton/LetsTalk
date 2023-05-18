#include <cstdlib>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <iostream>

#include "LetsTalk.hpp"
#include "LetsTalkFwd.hpp"

namespace lt {

namespace efd = eprosima::fastdds::dds;
namespace efr = eprosima::fastrtps::rtps;

// The default profile is a string constant stored elsewhere
std::string getDefaultProfileXml();

Guid Publisher::guid() const
{
    return toLetsTalkGuid(m_writer->guid());
}

ParticipantPtr Participant::create(uint8_t i_domain, std::string const& i_qosProfile)
{
    auto factory = efd::DomainParticipantFactory::get_instance();

    // Load profiles if we haven't yet
    static bool s_loadedProfiles = false;
    if (!s_loadedProfiles) {
        char const* profileXml = nullptr;
        profileXml = getenv("LT_PROFILE");
        if (profileXml) {
            auto code = factory->load_XML_profiles_file(profileXml);
            LT_LOG << "QOS profile xml loaded with code " << code;
        } else {
            std::string defaultXml = getDefaultProfileXml();
            factory->load_XML_profiles_string(defaultXml.c_str(), defaultXml.size());
        }

        // Adjust the name if it is the default or marked to be updated
        auto qos = factory->get_default_participant_qos();
        if (qos.name().size() == 0 || qos.name()[0] == '@' || strncmp(qos.name().c_str(), "RTPSParticipant", 15) == 0) {
            qos.name(program_invocation_short_name);
            factory->set_default_participant_qos(qos);
        }
        s_loadedProfiles = true;
    }

    // Create the participant
    efd::DomainParticipant* rawParticipant;
    efd::DomainParticipantQos qos = factory->get_default_participant_qos();
    if (!i_qosProfile.empty()) {
        auto status = factory->get_participant_qos_from_profile(i_qosProfile, qos);
        if (status != ReturnCode_t::RETCODE_OK) {
            LT_LOG << "Could not get participant QOS profile \"" << i_qosProfile << "\"\n";
            qos = factory->get_default_participant_qos();
        }
    }

    // RAII design to bind up the factory deletion methods with the dtors in shared_ptr.
    // 1. Create an entity from the factory
    // 2. Make a lambda to delete it on the factory
    // 3. Put these together in a shared_ptr

    rawParticipant = factory->create_participant(i_domain, qos);
    auto participantDeleter = [factory](efd::DomainParticipant* raw) {
        auto d1 = raw->delete_contained_entities();
        auto d2 = factory->delete_participant(raw);
    };
    auto participant = std::shared_ptr<efd::DomainParticipant>(rawParticipant, participantDeleter);

    // Create the publisher
    auto rawPub = rawParticipant->create_publisher(rawParticipant->get_default_publisher_qos());
    auto pubDeleter = [participant](efd::Publisher* pub) {
        pub->delete_contained_entities();
        participant->delete_publisher(pub);
    };
    auto publisher = std::shared_ptr<efd::Publisher>(rawPub, pubDeleter);

    // Create the subscriber
    auto rawSub = rawParticipant->create_subscriber(rawParticipant->get_default_subscriber_qos());
    auto subDeleter = [participant](efd::Subscriber* sub) {
        sub->delete_contained_entities();
        participant->delete_subscriber(sub);
    };
    auto subscriber = std::shared_ptr<efd::Subscriber>(rawSub, subDeleter);

    // Make the lt::Participant instance
    auto p = std::make_shared<Participant>();
    p->m_participant = participant;
    p->m_publisher = publisher;
    p->m_subscriber = subscriber;

    // Make the participant listener for logging and stat keeping. Note the mask needs to
    // be set or all events are eaten by the participant listener
    efd::StatusMask mask = efd::StatusMask::all();
    mask >> efd::StatusMask::data_available() >> efd::StatusMask::data_on_readers();
    mask << efd::StatusMask::subscription_matched() << efd::StatusMask::publication_matched();
    p->m_participant->set_listener(new detail::ParticipantLogger(p), mask);
    return p;
}

Participant::~Participant()
{
    m_publisher.reset();
    m_subscriber.reset();
    m_participant.reset();
}

std::string Participant::name() const
{
    return m_participant->get_qos().name().c_str();
}

Publisher Participant::doAdvertise(std::string const& i_topic, efd::TypeSupport const& i_type,
                                   std::string const& i_qosProfile, int i_historyDepth)
{
    auto topic = getTopic(i_topic, i_type, i_historyDepth);
    if (nullptr == topic) { return Publisher(nullptr, "null"); }
    efd::DataWriterQos qos = m_publisher->get_default_datawriter_qos();
    if (!i_qosProfile.empty()) {
        auto status = m_publisher->get_datawriter_qos_from_profile(i_qosProfile, qos);
        if (status != ReturnCode_t::RETCODE_OK) {
            LT_LOG << m_participant << " error setting qos profile \"" << i_qosProfile << "\"\n";
            qos = m_publisher->get_default_datawriter_qos();
        }
    }
    m_publisher->copy_from_topic_qos(qos, topic->get_qos());

    // Follow the pattern of binding the raw object with its deleter in a shared_ptr
    efd::DataWriter* rawWriter = m_publisher->create_datawriter(topic, qos);
    auto writerDeleter = [this](efd::DataWriter* raw) { m_publisher->delete_datawriter(raw); };
    auto writer = std::shared_ptr<efd::DataWriter>(rawWriter, writerDeleter);
    LT_LOG << m_participant << " created new publisher for type \"" << i_type.get_type_name() << "\" on topic \""
           << i_topic << "\"\n";
    return Publisher(writer, i_topic);
}

void Participant::doSubscribe(std::string const& i_topic, efd::TypeSupport const& i_type,
                              efd::DataReaderListener* i_listener, std::string const& i_qosProfile, int i_historyDepth)
{
    // Only one reader per topic, please
    auto reader = m_subscriber->lookup_datareader(i_topic);
    if (reader && reader->type().get_type_name() != i_type.get_type_name()) {
        auto code = m_subscriber->delete_datareader(reader);
        LT_LOG << "Deleted old datareader on " << i_topic << "; " << code << "\n";
    }

    // Ensure the topic exists with the correct type
    auto topic = getTopic(i_topic, i_type, i_historyDepth);
    if (topic == nullptr) {
        LT_LOG << "Error: Could not create topic " << i_topic << "\n";
        return;
    }

    // Get the QoS if required
    efd::DataReaderQos qos = m_subscriber->get_default_datareader_qos();
    if (!i_qosProfile.empty()) {
        auto status = m_subscriber->get_datareader_qos_from_profile(i_qosProfile, qos);
        if (status != ReturnCode_t::RETCODE_OK) {
            LT_LOG << m_participant << " error setting qos profile \"" << i_qosProfile << "\"\n";
            qos = m_subscriber->get_default_datareader_qos();
        }
    }

    // Make the data reader. This entity is kept alive by the subscriber
    m_subscriber->copy_from_topic_qos(qos, topic->get_qos());
    reader = m_subscriber->create_datareader(topic, qos, i_listener, efd::StatusMask::data_available());
    LT_LOG << m_participant << " created new subscriber for type \"" << i_type->getName() << "\" on topic \"" << i_topic
           << "\"\n";
}

// Since we don't keep an instance of the reader, we'll look it up on demand
void Participant::unsubscribe(std::string const& i_topic)
{
    auto reader = m_subscriber->lookup_datareader(i_topic);
    if (reader) {
        reader->delete_contained_entities();
        auto code = m_subscriber->delete_datareader(reader);
        LT_LOG << m_participant << " Unsubscribed from " << i_topic << "; " << code << "\n";
    }
}

// Technically writer instances are kept in the publisher, but we can delete them out from
// under that object here
void Participant::unadvertise(std::string const& i_service)
{
    auto server = m_subscriber->lookup_datareader(detail::requestName(i_service));
    if (server) {
        server->delete_contained_entities();
        auto code = m_subscriber->delete_datareader(server);
        LT_LOG << m_participant << " Unadverised service " << i_service << "; " << code << "\n";
    }
    auto replier = m_publisher->lookup_datawriter(detail::replyName(i_service));
    if (replier) { m_publisher->delete_datawriter(replier); }
}

// Callback to update the table of counts
void Participant::updatePublisherCount(std::string const& i_topic, int i_update)
{
    std::unique_lock<std::mutex> guard(m_countMutex);
    auto it = m_publisherCount.find(i_topic);
    if (it == m_publisherCount.end()) {
        m_publisherCount[i_topic] = i_update;
    } else {
        it->second += i_update;
    }
}

// Callback to update the table of counts
void Participant::updateSubscriberCount(std::string const& i_topic, int i_update)
{
    std::unique_lock<std::mutex> guard(m_countMutex);
    auto it = m_subscriberCount.find(i_topic);
    if (it == m_subscriberCount.end()) {
        m_subscriberCount[i_topic] = i_update;
    } else {
        it->second += i_update;
    }
}

// Get the count. Note the mutex
int Participant::publisherCount(std::string const& i_topic) const
{
    std::unique_lock<std::mutex> guard(m_countMutex);
    auto it = m_publisherCount.find(i_topic);
    if (it == m_publisherCount.end()) {
        return 0;
    } else {
        return it->second;
    }
}

// Get the count. Note the mutex
int Participant::subscriberCount(std::string const& i_topic) const
{
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

std::string Participant::topicType(std::string const& i_topic) const
{
    auto topic = m_participant->find_topic(i_topic, eprosima::fastrtps::Duration_t(0, 10000));
    if (nullptr == topic) { return std::string(); }
    std::string type_name = topic->get_type_name();
    m_participant->delete_topic(topic);
    return type_name;
}

efd::Topic* Participant::getTopic(std::string const& i_topic, efd::TypeSupport const& i_type, int i_historyDepth)
{
    auto qos = m_participant->get_default_topic_qos();
    efd::HistoryQosPolicy& history = qos.history();
    history.kind = efd::KEEP_LAST_HISTORY_QOS;
    history.depth = i_historyDepth;

    auto topic = m_participant->find_topic(i_topic, eprosima::fastrtps::Duration_t(0, 10000));
    bool foundIt = (topic != nullptr);
    if (!foundIt) {
        registerType(i_type);
        topic = m_participant->create_topic(i_topic, i_type.get_type_name(), qos);
        LT_LOG << m_participant << " started a new topic for type " << i_type.get_type_name() << " named \"" << i_topic
               << "\"\n";
    }
    if (nullptr == topic) {
        LT_LOG << m_participant << " could not create topic \"" << i_topic << "\"\n";
    } else if (topic->get_type_name() != i_type.get_type_name()) {
        LT_LOG << m_participant << " already has a topic \"" << i_topic << "\", but for type " << topic->get_type_name()
               << " not the requested type " << i_type.get_type_name() << "\n";
        if (foundIt) { m_participant->delete_topic(topic); }
        topic = nullptr;
    } else if (topic->get_qos().history().depth != i_historyDepth) {
        LT_LOG << m_participant << " already has topic \"" << i_topic << "\", but history depth is "
               << topic->get_qos().history().depth << " instead of requested depth of " << i_historyDepth << "\n";
        if (foundIt) { m_participant->delete_topic(topic); }
        topic = nullptr;
    }
    return topic;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Publisher definitions for untemplated methods

Publisher::Publisher(std::shared_ptr<efd::DataWriter> i_writer, std::string const& i_topicName)
    : m_writer(i_writer), m_topicName(i_topicName)
{
}

bool Publisher::doPublish(void* i_data)
{
    if (!isOkay() || nullptr == i_data) {
        LT_LOG << m_writer << " could not publish sample " << i_data << "\n";
        return false;
    }
    return m_writer->write(i_data);
}

bool Publisher::doPublish(void* i_data, Guid const& i_myId, Guid const& i_relatedId, bool i_bad)
{
    if (!isOkay() || nullptr == i_data) {
        LT_LOG << m_writer << " could not publish sample " << i_data << "\n";
        return false;
    }
    efr::WriteParams i_correlation;
    i_correlation.sample_identity(toSampleId(i_myId));
    if (i_bad) {
        i_correlation.related_sample_identity(toSampleId(i_relatedId.makeBadVersion()));
    } else {
        i_correlation.related_sample_identity(toSampleId(i_relatedId));
    }
    return m_writer->write(i_data, i_correlation);
}

}  // namespace lt
