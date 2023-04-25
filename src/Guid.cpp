#include "Guid.hpp"

#include <iostream>

namespace lt {

Guid Guid::makeBadVersion() const
{
  Guid bad(*this);
  bad.sequence = ~bad.sequence;
  return bad;
}

bool Guid::isBadVersionOf(Guid const& i_other) const
{
  Guid i_twin = i_other.makeBadVersion();
  return (i_twin == *this);
}

efr::GUID_t toFastDdsGuid(Guid const& i_guid)
{
  efr::GUID_t fastGuid;
  memcpy(fastGuid.guidPrefix.value, i_guid.data, efr::GuidPrefix_t::size);
  memcpy(fastGuid.entityId.value, &i_guid.data[12], efr::EntityId_t::size);
  return fastGuid;
}

Guid toLetsTalkGuid(efr::GUID_t const& i_guid)
{
  Guid lib;
  memcpy(lib.data, i_guid.guidPrefix.value, efr::GuidPrefix_t::size);
  memcpy(&lib.data[efr::GuidPrefix_t::size], i_guid.entityId.value, i_guid.entityId.size);
  return lib;
}

efr::SampleIdentity toSampleId(Guid const& i_id)
{
  efr::SampleIdentity id;
  id.writer_guid(toFastDdsGuid(i_id));
  id.sequence_number(efr::SequenceNumber_t(i_id.sequence));
  return id;
}

Guid toLetsTalkGuid(efr::SampleIdentity const& i_sampleId)
{
  Guid guid = toLetsTalkGuid(i_sampleId.writer_guid());
  guid.sequence = i_sampleId.sequence_number().to64long();
  return guid;
}

std::ostream& operator<<(std::ostream& os, Guid const& i_guid)
{
  uint64_t const* first = reinterpret_cast<uint64_t const*>(&i_guid.data[0]);
  uint64_t const* second = reinterpret_cast<uint64_t const*>(&i_guid.data[8]);
  return os << *first << "-" << *second << "-" << i_guid.sequence;
}

}  // namespace lt
