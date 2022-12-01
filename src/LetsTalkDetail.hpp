#pragma once
#include "LetsTalk.hpp"
#include "assert.h"
#include "get_demangled_name.hpp"

namespace lt {
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
    logSubscriptionMatched(m_participant, m_topic, info);
}

template<class T, class C>
void ReaderListener<T,C>::on_data_available(::eprosima::fastdds::dds::DataReader* i_reader) {
    namespace efd = eprosima::fastdds::dds;
    std::unique_ptr<T> data(new T);
    efd::SampleInfo info;
    if (i_reader->take_next_sample(data.get(), &info) == eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK) {
        if (info.valid_data) {
            m_callback(std::move(data));
        }
    }
}

}


template<class T>
bool Publisher::publish(std::unique_ptr<T> i_data) {
    assert(m_serializer->getName() == get_demangled_name<T>());
    return doPublish(i_data.get());
}
}
