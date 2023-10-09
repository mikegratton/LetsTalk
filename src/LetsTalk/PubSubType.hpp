#pragma once

#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>
// #include <fastdds/rtps/common/SerializedPayload.h>
#include <fastrtps/utils/md5.h>

#include <fastcdr/CdrSizeCalculator.hpp>
// #include <fastdds/dds/topic/TopicDataType.hpp>

#include "LetsTalkFwd.hpp"
#include "fastdds/dds/core/policy/QosPolicies.hpp"

namespace eprosima {
namespace fastcdr {
template <class T>
class CdrTypeProperties {
};
}  // namespace fastcdr
}  // namespace eprosima

namespace lt {

namespace detail {

template <class T>
class PubSubType : public eprosima::fastdds::dds::TopicDataType {
   public:
    using DataRepresentationId_t = ::eprosima::fastdds::dds::DataRepresentationId_t;
    using SerializedPayload_t = ::eprosima::fastrtps::rtps::SerializedPayload_t;
    typedef T type;

    PubSubType();

    ~PubSubType() override
    {
        if (m_keyBuffer != nullptr) { free(m_keyBuffer); }
    }

    bool serialize(void* data, SerializedPayload_t* payload, DataRepresentationId_t data_representation) override;

    bool serialize(void* data, SerializedPayload_t* payload) override
    {
        return serialize(data, payload, eprosima::fastdds::dds::DEFAULT_DATA_REPRESENTATION);
    }

    bool deserialize(SerializedPayload_t* payload, void* data) override;

    std::function<uint32_t()> getSerializedSizeProvider(void* data) override
    {
        return getSerializedSizeProvider(data, eprosima::fastdds::dds::DEFAULT_DATA_REPRESENTATION);
    }

    std::function<uint32_t()> getSerializedSizeProvider(void* data,
                                                        DataRepresentationId_t data_representation) override;

    bool getKey(void* data, eprosima::fastrtps::rtps::InstanceHandle_t* ihandle, bool force_md5 = false) override;

    void* createData() override { return reinterpret_cast<void*>(new T()); }

    void deleteData(void* data) override { delete reinterpret_cast<T*>(data); }

#ifdef TOPIC_DATA_TYPE_API_HAS_IS_BOUNDED
    inline bool is_bounded() const override
    {
        return false;
    }
#endif  // TOPIC_DATA_TYPE_API_HAS_IS_BOUNDED

#ifdef TOPIC_DATA_TYPE_API_HAS_IS_PLAIN
    inline bool is_plain() const override
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

    MD5 m_md5;
    unsigned char* m_keyBuffer;
};

////////////////////////////////////////////

template <class T>
PubSubType<T>::PubSubType()
{
    using CdrTypeProperties = eprosima::fastcdr::CdrTypeProperties<T>;
    setName(detail::get_demangled_name<T>().c_str());
    auto type_size = CdrTypeProperties::kMaxCdrTypeSize;
    type_size += eprosima::fastcdr::Cdr::alignment(type_size, 4); /* possible submessage alignment */
    m_typeSize = static_cast<uint32_t>(type_size) + 4;            /*encapsulation*/
    m_isGetKeyDefined = CdrTypeProperties::kHasKey;
    size_t keyLength = CdrTypeProperties::kMaxKeyCdrTypeSize > 16 ? CdrTypeProperties::kMaxKeyCdrTypeSize : 16;
    m_keyBuffer = reinterpret_cast<unsigned char*>(malloc(keyLength));
    memset(m_keyBuffer, 0, keyLength);
}

template <class T>
bool PubSubType<T>::serialize(void* data, SerializedPayload_t* payload, DataRepresentationId_t data_representation)
{
    T* p_type = static_cast<T*>(data);

    eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload->data), payload->max_size);
    eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
                               data_representation == DataRepresentationId_t::XCDR_DATA_REPRESENTATION
                                   ? eprosima::fastcdr::CdrVersion::XCDRv1
                                   : eprosima::fastcdr::CdrVersion::XCDRv2);
    payload->encapsulation = ser.endianness() == ::eprosima::fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
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
    payload->length = static_cast<uint32_t>(ser.get_serialized_data_length());
    return true;
}

template <class T>
bool PubSubType<T>::deserialize(SerializedPayload_t* payload, void* data)
{
    try {
        T* p_type = static_cast<T*>(data);
        eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload->data), payload->length);
        eprosima::fastcdr::Cdr deser(fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN);
        deser.read_encapsulation();
        payload->encapsulation = deser.endianness() == eprosima::fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
        deser >> *p_type;
    } catch (eprosima::fastcdr::exception::NotEnoughMemoryException& /*exception*/) {
        return false;
    }

    return true;
}

template <class T>
inline std::function<uint32_t()> PubSubType<T>::getSerializedSizeProvider(void* data,
                                                                          DataRepresentationId_t data_representation)
{
    return [data, data_representation]() -> uint32_t {
        eprosima::fastcdr::CdrSizeCalculator calculator(data_representation ==
                                                                DataRepresentationId_t::XCDR_DATA_REPRESENTATION
                                                            ? eprosima::fastcdr::CdrVersion::XCDRv1
                                                            : eprosima::fastcdr::CdrVersion::XCDRv2);
        size_t current_alignment{0};
        return static_cast<uint32_t>(calculator.calculate_serialized_size(*static_cast<T*>(data), current_alignment)) +
               4u /*encapsulation*/;
    };
}

template <class T>
inline bool PubSubType<T>::getKey(void* data, ::eprosima::fastrtps::rtps::InstanceHandle_t* handle, bool force_md5)
{
    using CdrTypeProperties = eprosima::fastcdr::CdrTypeProperties<T>;
    if (!m_isGetKeyDefined) { return false; }

    T* p_type = static_cast<T*>(data);

    // Object that manages the raw buffer.
    eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(m_keyBuffer),
                                             CdrTypeProperties::kMaxKeyCdrTypeSize);

    // Object that serializes the data.
    eprosima::fastcdr::Cdr ser(fastbuffer, ::eprosima::fastcdr::Cdr::BIG_ENDIANNESS);
    CdrTypeProperties::serializeKey(ser, *p_type);
    if (force_md5 || CdrTypeProperties::kMaxKeyCdrTypeSize > 16) {
        m_md5.init();
        m_md5.update(m_keyBuffer, static_cast<unsigned int>(ser.get_serialized_data_length()));
        m_md5.finalize();
        memcpy(handle->value, m_md5.digest, 16);
    } else {
        memcpy(handle->value, m_keyBuffer, 16);
    }
    return true;
}
}  // namespace detail
}  // namespace lt
