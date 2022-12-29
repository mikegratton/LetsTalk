#pragma once
#include <map>
#include <vector>
#include <mutex>
#include "LetsTalkFwd.hpp"
#include "PubSubType.hpp"

namespace lt {


/**
 * Participant is a node in the pub/sub network.  It is responsible for discovering other nodes,
 * running callbacks on message reciept and handling the business of socket management.
 *
 * Participants are handled as shared pointers.
 */
class Participant : public std::enable_shared_from_this<Participant> {

    using TypeSupport = eprosima::fastdds::dds::TypeSupport;
    using Topic = eprosima::fastdds::dds::Topic;

public:

    /**
     * create() makes a new Participant
     *
     * @param i_domain Domains are logically isolated pub/sub communities. Valid values are
     *                 0 to 240
     * @param i_qosProfile The name a of a settings "QOS" (Quality of Service) profile.
     *                 Profiles are defined in XML.
     *
     */
    static ParticipantPtr create(int i_domain = 0, std::string const& i_qosProfile = "");
    ~Participant();


    /**
     * subscribe<T>(topic, callback, profile)
     *
     * Register a callback on the named topic expecting type T.  When data arrives, the callback
     * will be called.  Callbacks take the form
     * ```
     *   void my_callback(std::unique_ptr<T> new_data);
     * ```
     * Note that data is provided as a unique_ptr.
     *
     * @param i_topic Topic name to subscribe to
     *
     * @param i_callback Callback function or lambda.
     *
     * @param i_qosProfile Settings profile
     *
     * @param i_historyDepth Number of historical messages to store
     */
    template<class T, class C>
    void subscribe(std::string const& i_topic, C i_callback, std::string const& i_qosProfile = "",
                   int i_historyDepth=1);

    /**
     * unsubscribe(topic)
     *
     * Unsubscribe from the given topic if subscribed.
     *
     * @param i_topic Topic to unsubscribe from
     */
    void unsubscribe(std::string const& i_topic);


    /**
     * advertise<T>(topic)
     *
     * Get a Publisher object you can use to send messages of type T on the topic.
     *
     * @param i_topic Name of the topic
     *
     * @param i_qosProfile Settings profile
     *
     * Note: Publisher is a lightweight class that is cheap to copy. It's primary API
     * call is
     * ```
     * bool publish(std::unique_ptr<T> i_data);
     * ```
     * which returns true if the data was sent successfully.
     */
    template<class T>
    Publisher advertise(std::string const& i_topic, std::string const& i_qosProfile = "",
                        int i_historyDepth=1);

    /**
     * Obtain the current known number of publishers on a given topic
     */
    int publisherCount(std::string const& i_topic) const;

    /**
     * Obtain the current known number of subscribers on a given topic
     */
    int subscriberCount(std::string const& i_topic) const;

    /**
     * Get the name (demangled) of the type in use on a given topic
     */
    std::string topicType(std::string const& i_topic) const;

    /*
     * Expert interface to underlying fastdds components. These calls may be removed
     * in the future.
     */
    std::shared_ptr<efd::DomainParticipant> getRawParticipant() { return m_participant; }
    std::shared_ptr<efd::Publisher> getRawPublisher() { return m_publisher; }
    std::shared_ptr<efd::Subscriber> getRawSubscriber() { return m_subscriber; }

protected:

    /// Get a pointer to an existing topic, or create a new topic (registering i_type) and
    /// return a pointer to that. If a topic exists using a different type, it will be
    /// deleted, and a new topic created for the new (topic, type) pair.
    Topic* getTopic(std::string const& i_topic, TypeSupport const& i_type, int i_historyDepth=1);

    /// Register the serialize/deserialize support with the participant
    void registerType(TypeSupport const& i_type);

    /// Type-erased advertise function
    Publisher doAdvertise(std::string const& i_topic, TypeSupport const& i_type,
                          std::string const& i_qosProfile, int i_historyDepth);

    /// Type-erased subscribe function
    void doSubscribe(std::string const& i_topic, TypeSupport const& i_typeName,
                     efd::DataReaderListener* i_listener, std::string const& i_qosProfile,
                     int i_historyDepth);

    /// Callback for updating the pub/sub counts
    void updatePublisherCount(std::string const& i_topic, int i_update);
    void updateSubscriberCount(std::string const& i_topic, int i_update);

    /// Reader callbacks use this templated class
    template<class T, class C> friend class detail::ReaderListener;

    /// The publisher has a private ctor
    friend class Publisher;

    /// Allow the subscriber callbacks to update the count
    friend class detail::SubscriberCounter;

    std::shared_ptr<efd::DomainParticipant> m_participant;
    std::shared_ptr<efd::Publisher> m_publisher;
    std::shared_ptr<efd::Subscriber> m_subscriber;

    mutable std::mutex m_countMutex; // Guards the pub/sub count maps
    std::map<std::string, int> m_subscriberCount; // Number of readers per topic
    std::map<std::string, int> m_publisherCount; // Number of writers per topic
};


/*
 * A type-erased publisher object. This is used to send
 * data on a topic.  These are created by the Participant.
 */
class Publisher {
public:

    operator bool() const { return isOkay(); }
    bool isOkay() const { return !(nullptr == m_writer); }

    template<class T>
    bool publish(std::unique_ptr<T> i_data);

protected:
    friend Participant;

    Publisher(std::shared_ptr<efd::DataWriter> i_writer);
    bool doPublish(void* i_data);

    std::shared_ptr<efd::DataWriter> m_writer;
};

template<class T, class C>
void Participant::subscribe(std::string const& i_topic, C i_callback, std::string const& i_qosProfile,
               int i_historyDepth) {
    auto listener = detail::makeListener<T, C>(i_callback, i_topic, shared_from_this());
    doSubscribe(i_topic, TypeSupport(new PubSubType<T>()), listener, i_qosProfile, i_historyDepth);
}

template<class T>
Publisher Participant::advertise(std::string const& i_topic, std::string const& i_qosProfile,
                                 int i_historyDepth) {
    return doAdvertise(i_topic, TypeSupport(new PubSubType<T>()), i_qosProfile, i_historyDepth);
}

}

#include "LetsTalkDetail.hpp"
