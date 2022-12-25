#pragma once
#include "LetsTalk.hpp"
#include "assert.h"
#include <string>

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif

namespace lt {
namespace detail {


extern const bool LT_VERBOSE;

#define LT_LOG if(!::lt::detail::LT_VERBOSE){} else std::cout

using eprosima::fastrtps::types::ReturnCode_t;

std::string getDefaultProfileXml(char const* i_participantName = program_invocation_short_name);
    
template<class T, class C>
ReaderListener<T,C>::ReaderListener(C i_callback, std::string const& i_topic, ParticipantPtr i_participant)
    : m_callback(i_callback)
    , m_topic(i_topic)
    , m_participant(i_participant)
{ }


template<class T, class C>
void ReaderListener<T,C>::on_data_available(efd::DataReader* i_reader) {
    if (nullptr == i_reader) {
        LT_LOG << m_topic << " callback has a null reader\n";
        return;
    }
    std::unique_ptr<T> data(new T);
    efd::SampleInfo info;
    auto code = i_reader->read_next_sample(data.get(), &info);
    if ( code == ReturnCode_t::RETCODE_OK) {
        if (info.valid_data) {
            m_callback(std::move(data));
        }
    } else {
        LT_LOG << "Notified of data available on " << m_topic << ", but code is " << code << "\n";
    }
}

template<class T, class C>
void ReaderListener<T,C>::on_subscription_matched(efd::DataReader*,
        efd::SubscriptionMatchedStatus const& info) {
    if (m_participant) {
        m_participant->updatePublisherCount(m_topic, info.current_count_change);
    }
    logSubscriptionMatched(m_participant, m_topic, info);
}

template<class T, class C>
void ReaderListener<T,C>::on_sample_rejected(efd::DataReader* reader, 
                                             const efd::SampleRejectedStatus& status) 
{
    logSampleRejected(m_participant, m_topic, status);    
}

template<class T, class C>
void ReaderListener<T,C>::on_requested_incompatible_qos(efd::DataReader* reader, const efd::RequestedIncompatibleQosStatus& status) 
{
    logIncompatibleQos(m_participant, m_topic, status);    
}

template<class T, class C>
void ReaderListener<T,C>::on_sample_lost(efd::DataReader* reader, 
                                         const efd::SampleLostStatus& status) 
{
    logLostSample(m_participant, m_topic, status);    
}

#if defined(__GNUC__) || defined(__clang__)
template<class T>
std::string get_demangled_name()
{
    char* realname = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    std::string rname(realname);
    free(realname);
    return rname;
}
#else
template<class T>
std::string get_demangled_name()
{
    return std::string(typeid(T).name());
}
#endif


}


template<class T>
bool Publisher::publish(std::unique_ptr<T> i_data) {
    assert(m_serializer->getName() == get_demangled_name<T>());    
    return doPublish(i_data.get());
}
}
