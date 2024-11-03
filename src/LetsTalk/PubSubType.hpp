#pragma once

#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>

#include <fastcdr/CdrSizeCalculator.hpp>
#include <fastdds/rtps/common/SerializedPayload.hpp>
#include <fastdds/utils/md5.hpp>
// #include <fastdds/dds/topic/TopicDataType.hpp>

#include "fastdds/dds/core/policy/QosPolicies.hpp"

namespace eprosima {
namespace fastcdr {
template <class T>
struct CdrTypeProperties;
}
}  // namespace eprosima

namespace lt {

namespace detail {

template <class T>
class PubSubType : public eprosima::fastdds::dds::TopicDataType {
   public:
    using DataRepresentationId_t = ::eprosima::fastdds::dds::DataRepresentationId_t;
    using SerializedPayload_t = ::eprosima::fastdds::rtps::SerializedPayload_t;
    typedef T type;

    PubSubType();

    ~PubSubType() override
    {
        if (m_keyBuffer != nullptr) { free(m_keyBuffer); }
    }

    bool serialize(void const* const data, SerializedPayload_t& payload,
                   DataRepresentationId_t data_representation) override;

    bool deserialize(SerializedPayload_t& payload, void* data) override;

    uint32_t calculate_serialized_size(void const* const data,
                                       eprosima::fastdds::dds::DataRepresentationId_t data_representation);

    bool compute_key(SerializedPayload_t& payload, efr::InstanceHandle_t& handle, bool force_md5 = false) override;

    bool compute_key(void const* const data, efr::InstanceHandle_t& ihandle, bool force_md5 = false) override;

    void* create_data() override { return reinterpret_cast<void*>(new T()); }

    void delete_data(void* data) override { delete reinterpret_cast<T*>(data); }

#ifdef TOPIC_DATA_TYPE_API_HAS_IS_BOUNDED
    inline bool is_bounded() const override
    {
        return false;
    }
#endif  // TOPIC_DATA_TYPE_API_HAS_IS_BOUNDED

#ifdef TOPIC_DATA_TYPE_API_HAS_IS_PLAIN
    inline bool is_plain(DataRepresentationId_t) const override
    {
        return false;
    }
#endif  // TOPIC_DATA_TYPE_API_HAS_IS_PLAIN

#ifdef TOPIC_DATA_TYPE_API_HAS_CONSTRUCT_SAMPLE
    inline bool construct_sample(void* memory) const override
    {
        (void)memory;
        return false;
    }
#endif  // TOPIC_DATA_TYPE_API_HAS_CONSTRUCT_SAMPLE

    eprosima::fastdds::MD5 m_md5;
    unsigned char* m_keyBuffer;
};

////////////////////////////////////////////

template <class T>
PubSubType<T>::PubSubType()
{
    using CdrTypeProperties = eprosima::fastcdr::CdrTypeProperties<T>;
    set_name(CdrTypeProperties::typeName());
    auto type_size = CdrTypeProperties::kMaxCdrTypeSize;
    type_size += eprosima::fastcdr::Cdr::alignment(type_size, 4);    /* possible submessage alignment */
    max_serialized_type_size = static_cast<uint32_t>(type_size) + 4; /*encapsulation*/
    is_compute_key_provided = (CdrTypeProperties::kMaxKeyCdrTypeSize > 0);
    size_t keyLength = CdrTypeProperties::kMaxKeyCdrTypeSize > 16 ? CdrTypeProperties::kMaxKeyCdrTypeSize : 16;
    m_keyBuffer = reinterpret_cast<unsigned char*>(malloc(keyLength));
    memset(m_keyBuffer, 0, keyLength);
}

template <class T>
bool PubSubType<T>::serialize(void const* const data, SerializedPayload_t& payload,
                              DataRepresentationId_t data_representation)
{
    T const* p_type = static_cast<T const*>(data);

    eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload.data), payload.max_size);
    eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
                               data_representation == DataRepresentationId_t::XCDR_DATA_REPRESENTATION
                                   ? eprosima::fastcdr::CdrVersion::XCDRv1
                                   : eprosima::fastcdr::CdrVersion::XCDRv2);
    payload.encapsulation = ser.endianness() == ::eprosima::fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
    ser.set_encoding_flag(data_representation == DataRepresentationId_t::XCDR_DATA_REPRESENTATION
                              ? eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR
                              : eprosima::fastcdr::EncodingAlgorithmFlag::DELIMIT_CDR2);

    try {
        ser.serialize_encapsulation();
        ser << *p_type;
    } catch (eprosima::fastcdr::exception::NotEnoughMemoryException& /*exception*/) {
        return false;
    }

    // Get the serialized length
    payload.length = static_cast<uint32_t>(ser.get_serialized_data_length());
    return true;
}

template <class T>
bool PubSubType<T>::deserialize(SerializedPayload_t& payload, void* data)
{
    try {
        T* p_type = static_cast<T*>(data);
        eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload.data), payload.length);
        eprosima::fastcdr::Cdr deser(fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN);
        deser.read_encapsulation();
        payload.encapsulation = deser.endianness() == eprosima::fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
        deser >> *p_type;
    } catch (eprosima::fastcdr::exception::NotEnoughMemoryException& /*exception*/) {
        return false;
    }

    return true;
}

template <class T>
inline uint32_t PubSubType<T>::calculate_serialized_size(
    void const* const data, eprosima::fastdds::dds::DataRepresentationId_t data_representation)
{
    try {
        eprosima::fastcdr::CdrSizeCalculator calculator(data_representation ==
                                                                DataRepresentationId_t::XCDR_DATA_REPRESENTATION
                                                            ? eprosima::fastcdr::CdrVersion::XCDRv1
                                                            : eprosima::fastcdr::CdrVersion::XCDRv2);
        size_t current_alignment{0};
        return static_cast<uint32_t>(
                   calculator.calculate_serialized_size(*static_cast<T const*>(data), current_alignment)) +
               4u /*encapsulation*/;
    } catch (eprosima::fastcdr::exception::Exception const&) {
        return 0;
    }
}

template <class T, bool DoIt>
struct ComputeKey {
    bool operator()(T const* p_type, efd::InstanceHandle_t& handle, bool force_md5, eprosima::fastdds::MD5& md5,
                    unsigned char* keyBuffer, uint32_t maxKeySize);
};

template <class T>
struct ComputeKey<T, false> {
    bool operator()(T const* p_type, efd::InstanceHandle_t& handle, bool force_md5, eprosima::fastdds::MD5& md5,
                    unsigned char* keyBuffer, uint32_t maxKeySize)
    {
        return false;
    }
};

template <class T>
struct ComputeKey<T, true> {
    inline bool operator()(T const* p_type, efd::InstanceHandle_t& handle, bool force_md5, eprosima::fastdds::MD5& md5,
                           unsigned char* keyBuffer, uint32_t maxKeySize)
    {
        // Object that manages the raw buffer.
        eprosima::fastcdr::FastBuffer fastbuffer(keyBuffer, maxKeySize);

        // Object that serializes the data.
        eprosima::fastcdr::Cdr ser(fastbuffer, ::eprosima::fastcdr::Cdr::BIG_ENDIANNESS,
                                   eprosima::fastcdr::CdrVersion::XCDRv2);
        ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR2);
        eprosima::fastcdr::serialize_key(ser, *p_type);
        if (force_md5 || maxKeySize > 16) {
            md5.init();
            md5.update(keyBuffer, static_cast<unsigned int>(ser.get_serialized_data_length()));
            md5.finalize();
            for (uint8_t i = 0; i < 16; ++i) { handle.value[i] = md5.digest[i]; }
        } else {
            for (uint8_t i = 0; i < 16; ++i) { handle.value[i] = keyBuffer[i]; }
        }
        return true;
    }
};

template <class T>
inline bool PubSubType<T>::compute_key(void const* data, efd::InstanceHandle_t& handle, bool force_md5)
{
    using CdrTypeProperties = eprosima::fastcdr::CdrTypeProperties<T>;
    T const* p_type = static_cast<T const*>(data);
    return detail::ComputeKey<T, (CdrTypeProperties::kMaxKeyCdrTypeSize > 0)>{}(
        p_type, handle, force_md5, m_md5, m_keyBuffer, CdrTypeProperties::kMaxKeyCdrTypeSize);
}

template <class T>
inline bool PubSubType<T>::compute_key(SerializedPayload_t& payload, efr::InstanceHandle_t& handle, bool force_md5)
{
    if (!is_compute_key_provided) { return false; }

    T data;
    if (deserialize(payload, static_cast<void*>(&data))) {
        return compute_key(static_cast<void*>(&data), handle, force_md5);
    }

    return false;
}

}  // namespace detail
}  // namespace lt
