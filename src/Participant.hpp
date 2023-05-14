#pragma once
#include <future>
#include <map>
#include <mutex>
#include <vector>

#include "LetsTalkFwd.hpp"
#include "PubSubType.hpp"
#include "Reactor.hpp"
#include "RequestReply.hpp"
#include "ThreadSafeQueue.hpp"
namespace lt {

/**
 * Participant is a node in the pub/sub network.  It is responsible for discovering other nodes,
 * running callbacks on message reciept and handling the business of socket management.
 *
 * Participants are handled as shared pointers.
 */
class Participant : public std::enable_shared_from_this<Participant> {
   public:
    /**
     * create() makes a new Participant.
     *
     * @param i_domain Domains are logically isolated pub/sub communities. Valid values are
     *                 0 to 232
     * @param i_qosProfile The name a of a settings "QOS" (Quality of Service) profile.
     *                 Profiles are defined in XML.
     *
     * @return pointer to the created participant
     *
     */
    static ParticipantPtr create(uint8_t i_domain = 0, std::string const& i_qosProfile = "");
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
    template <class T, class C>
    void subscribe(std::string const& i_topic, C i_callback, std::string const& i_qosProfile = "",
                   int i_historyDepth = 1);

    /**
     * subscribe<T>(topic, profile, history)
     *
     * Obtain a pointer to a shared queue of provided data.  Data will be placed in the queue, up
     * to the history level, at which point the oldest sample will be discarded.
     *
     * @param i_topic Topic name to subscribe to
     *
     * @param i_qosProfile Settings profile
     *
     * @param i_historyDepth Number of historical messages to store
     *
     * @return thread safe queue pointer where samples will appear
     */
    template <class T>
    QueuePtr<T> subscribe(std::string const& i_topic, std::string const& i_qosProfile = "", int i_historyDepth = 8);

    /**
     * unsubscribe(topic)
     *
     * Unsubscribe from the given topic if subscribed.
     *
     * @param i_topic Topic to unsubscribe from
     */
    void unsubscribe(std::string const& i_topic);

    /**
     * unadvertise(service)
     *
     * Stop providing the named service
     *
     * @param i_service Service to discontinue
     */
    void unadvertise(std::string const& i_service);

    /**
     * advertise<T>(topic)
     *
     * Get a Publisher object you can use to send messages of type T on the topic.
     *
     * @param i_topic Name of the topic
     *
     * @param i_qosProfile Settings profile
     *
     * @return lightweight publisher object. Note that the publish() calls will fail
     *   if the supplied data isn't of type T
     *
     * Note: Publisher is a lightweight class that is cheap to copy. It's primary API
     * call is
     * ```
     * bool publish(std::unique_ptr<T> i_data);
     * ```
     * which returns true if the data was sent successfully.
     */
    template <class T>
    Publisher advertise(std::string const& i_topic, std::string const& i_qosProfile = "", int i_historyDepth = 1);

    /**
     * Advertise a new request/reply service. This will create a new work thread and call i_serviceProvider
     * on each recieved Req to produce a Rep, then publish the Rep. If i_serviceProvider throws an exception,
     * this failure will be communicated back to the requester.
     *
     * @param i_serviceName Name of the service for determining the related topics
     *
     * @param i_serviceProvider a function object with a call of the form `Rep = C(Req)`
     *    This object may throw exceptions if Rep cannot be computed.
     */
    template <class Req, class Rep, class C>
    void advertise(std::string const& i_serviceName, C i_serviceProvider);

    /**
     * Build a requester object for making requests to a service. Each request
     * is an object of type Req and will correspond to one response object of
     * type std::promise<Rep>.
     *
     * Typically, this will look like
     * ```
     * auto requester = participant->request<Location,Temperature>("weatherService");
     * Location location(41.0, 70.0);
     * auto temp = requester.request(location).get();
     * ```
     *
     * @param i_serviceName Name of the service for determining the related topics
     *
     * @return a lightweight Requester object that can be used to send requests.
     *
     */
    template <class Req, class Rep>
    Requester<Req, Rep> makeRequester(std::string const& i_serviceName);

    /**
     * Make a reactor server object. This is a lightweight object that may be
     * cheaply copied.
     *
     * The ReactorServer handles the server-side of the reactor pattern. This is a pull-style API,
     * where you are responsible for polling the server's methods looking for new sessions.
     *
     * @param i_serviceName Used to calculate all of the related topics
     * @return ReactorServer instance
     */
    template <class Req, class Rep, class P = reactor_void_progress>
    ReactorServer<Req, Rep, P> makeReactorServer(std::string const& i_serviceName);

    /**
     * Make a reactor client object. This is a lightweight object that may be
     * cheaply copied.
     *
     * The ReactorClient controls the client side of a reactor. It can start multiple sessions, possibly
     * active simultaneously.
     *
     * The workflow is
     * 1. Send a request, starting a new session.
     * 2. Use the session object to check on progress.
     * 3. Get the reply, waiting as necessary.
     *
     * @param i_serviceName Used to calculate all of the related topics
     * @return ReactorClient instance
     */
    template <class Req, class Rep, class P = reactor_void_progress>
    ReactorClient<Req, Rep, P> makeReactorClient(std::string const& i_serviceName);

    /**
     * Obtain the current known number of publishers on a given topic
     *
     * @param i_topic Topic to query
     *
     * @return number of known publishers on i_topic (excluding this participant)
     */
    int publisherCount(std::string const& i_topic) const;

    /**
     * Obtain the current known number of subscribers on a given topic
     *
     * @param i_topic Topic to query
     *
     * @return number of known subscribers on i_topic (excluding this participant)
     */
    int subscriberCount(std::string const& i_topic) const;

    /**
     * Get the name (demangled) of the type in use on a given topic
     *
     * @param i_topic Topic to query
     *
     * @return Name of the type in use in i_topic
     */
    std::string topicType(std::string const& i_topic) const;

    /**
     * Get the name of the participant
     *
     * @return Name of the participant
     */
    std::string name() const;

   protected:
    /// Get a pointer to an existing topic, or create a new topic (registering i_type) and
    /// return a pointer to that. If a topic exists using a different type, it will be
    /// deleted, and a new topic created for the new (topic, type) pair.
    efd::Topic* getTopic(std::string const& i_topic, efd::TypeSupport const& i_type, int i_historyDepth = 1);

    /// Register the serialize/deserialize support with the participant
    void registerType(efd::TypeSupport const& i_type);

    /// Type-erased advertise function
    Publisher doAdvertise(std::string const& i_topic, efd::TypeSupport const& i_type, std::string const& i_qosProfile,
                          int i_historyDepth);

    /// Type-erased subscribe function
    void doSubscribe(std::string const& i_topic, efd::TypeSupport const& i_typeName,
                     efd::DataReaderListener* i_listener, std::string const& i_qosProfile, int i_historyDepth);

    /// Callback for updating the pub/sub counts
    void updatePublisherCount(std::string const& i_topic, int i_update);
    void updateSubscriberCount(std::string const& i_topic, int i_update);

    /// Reader callbacks use this templated class
    template <class T, class C>
    friend class detail::ReaderListener;

    /// The publisher has a private ctor
    friend class Publisher;
    template <class Req, class Rep>
    friend class Requester;

    /// Allow the participant callbacks to update the count
    friend class detail::ParticipantLogger;

    std::shared_ptr<efd::DomainParticipant> m_participant;  // Underlying DDS participant
    std::shared_ptr<efd::Publisher> m_publisher;            // Single pub object for all writers
    std::shared_ptr<efd::Subscriber> m_subscriber;          // Single sub object for all readers

    mutable std::mutex m_countMutex;               // Guards the pub/sub count maps
    std::map<std::string, int> m_subscriberCount;  // Number of readers per topic
    std::map<std::string, int> m_publisherCount;   // Number of writers per topic
};

/*
 * A type-erased lightweight publisher object used to send  data on a topic.
 * These are created by the Participant and may be cheaply copied.
 */
class Publisher {
   public:
    /**
     * Check that this object can send data
     */
    operator bool() const { return isOkay(); }
    bool isOkay() const { return !(nullptr == m_writer); }

    /**
     * Take the data sample and publish it.
     */
    template <class T>
    bool publish(std::unique_ptr<T> i_data);

    template <class T>
    bool publish(T const& i_data);

    /**
     * Publish with a given ID, related ID, and a good/bad flag. This is
     * mainly used by the request/response and reactors.
     */
    template <class T>
    bool publish(T const& i_data, Guid const& i_myId, Guid const& i_relatedId, bool i_bad = false);
    template <class T>
    bool publish(std::unique_ptr<T> i_data, Guid const& i_myId, Guid const& i_relatedId, bool i_bad = false);

    std::string const& topic() const { return m_topicName; }

    Guid guid() const;

    // Makes a dead publisher
    Publisher() = default;

   protected:
    friend class Participant;

    Publisher(std::shared_ptr<efd::DataWriter> i_writer, std::string const& i_topicName);

    /// Type-erased publish method
    bool doPublish(void* i_data);

    /// Type-erased publish method
    bool doPublish(void* i_data, Guid const& i_myId, Guid const& i_relatedId, bool i_bad);

    std::shared_ptr<efd::DataWriter> m_writer;
    std::string m_topicName;
};

}  // namespace lt

#include "ParticipantImpl.hpp"
