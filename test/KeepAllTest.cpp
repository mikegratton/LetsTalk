#include <chrono>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <thread>

#include "LetsTalk/LetsTalk.hpp"
#include "doctest.h"
#include "fastdds/dds/domain/qos/DomainParticipantQos.hpp"
#include "fastdds/dds/subscriber/DataReaderListener.hpp"
#include "fastdds/dds/subscriber/qos/SubscriberQos.hpp"
#include "idl/HelloWorld.h"

namespace efd = eprosima::fastdds::dds;
namespace efr = eprosima::fastrtps::rtps;

std::atomic<int> recCount = 0;

class Listener : public efd::DataReaderListener {
    void on_data_available(efd::DataReader* i_reader) final
    {
        HelloWorld data;
        efd::SampleInfo info;
        while (ReturnCode_t::RETCODE_OK == i_reader->take_next_sample(&data, &info)) {
            if (info.valid_data) {
                recCount++;
                std::cout << data.index() << ": " << data.message() << "\n";
            }
        }
    }
};

TEST_CASE("KeepAll")
{
    auto factory = efd::DomainParticipantFactory::get_instance();
    efd::DomainParticipant* subpart = factory->create_participant(0, efd::PARTICIPANT_QOS_DEFAULT);
    efd::DomainParticipant* pubpart1 = factory->create_participant(0, efd::PARTICIPANT_QOS_DEFAULT);
    efd::DomainParticipant* pubpart2 = factory->create_participant(0, efd::PARTICIPANT_QOS_DEFAULT);
    auto rawPub1 = pubpart1->create_publisher(efd::PUBLISHER_QOS_DEFAULT);
    auto rawPub2 = pubpart2->create_publisher(efd::PUBLISHER_QOS_DEFAULT);
    auto rawSub = subpart->create_subscriber(efd::SUBSCRIBER_QOS_DEFAULT);

    efd::TypeSupport typeThing(new lt::detail::PubSubType<HelloWorld>);
    typeThing.register_type(subpart);
    typeThing.register_type(pubpart1);
    typeThing.register_type(pubpart2);

    efd::TopicQos topicQos;
    topicQos.reliability().kind = efd::RELIABLE_RELIABILITY_QOS;
    topicQos.history().kind = efd::KEEP_ALL_HISTORY_QOS;
    auto topic = subpart->create_topic("hello", "HelloWorld", topicQos);

    efd::DataReaderQos readerQos;
    readerQos.reliability().kind = efd::RELIABLE_RELIABILITY_QOS;
    readerQos.history().kind = efd::KEEP_ALL_HISTORY_QOS;
    auto reader = rawSub->create_datareader(topic, readerQos);  // ?? Whose qos?
    reader->set_listener(new Listener());

    topic = pubpart1->create_topic("hello", "HelloWorld", topicQos);
    efd::DataWriterQos writerQos;
    writerQos.reliability().kind = efd::RELIABLE_RELIABILITY_QOS;
    writerQos.history().kind = efd::KEEP_ALL_HISTORY_QOS;
    auto writer1 = rawPub1->create_datawriter(topic, writerQos);

    topic = pubpart2->create_topic("hello", "HelloWorld", topicQos);
    auto writer2 = rawPub2->create_datawriter(topic, writerQos);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    HelloWorld sample;
    sample.index(0);
    sample.message("hello");
    writer1->write(&sample);
    sample.index(1);
    writer1->write(&sample);
    sample.index(2);
    writer1->write(&sample);
    sample.index(3);
    writer1->write(&sample);

    sample.index(0);
    sample.message("goodbye");
    writer2->write(&sample);
    sample.index(1);
    writer2->write(&sample);
    sample.index(2);
    writer2->write(&sample);
    sample.index(3);
    writer2->write(&sample);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(recCount == 8);
}