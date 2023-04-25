#pragma once
#include <cstdint>
#include <cstring>
#include <iosfwd>

#include "FastDdsAlias.hpp"

namespace lt {
struct Guid {
  unsigned char data[16];
  uint64_t sequence;
  Guid()
  {
    memset(data, 0, 16);
    sequence = 0;
  }
  static Guid UNKNOWN() { return Guid(); }
  void increment() { sequence += 1; }
  Guid makeBadVersion() const;
  bool isBadVersionOf(Guid const& i_other) const;
};

/////////////////////////////////////////////////////////////////////////////////
// Inline comparison operators for Guids
inline bool operator<(Guid const& g1, Guid const& g2)
{
  int compare = memcmp(g1.data, g2.data, sizeof(g1.data));
  return (compare < 0 || (compare == 0 && g1.sequence < g2.sequence));
}

inline bool operator==(Guid const& g1, Guid const& g2)
{
  return memcmp(g1.data, g2.data, sizeof(g1.data)) == 0 && g1.sequence == g2.sequence;
}

inline bool operator!=(Guid const& g1, Guid const& g2)
{
  return !(g1 == g2);
}

efr::GUID_t toFastDdsGuid(Guid const& i_guid);

Guid toLetsTalkGuid(efr::GUID_t const& i_guid);

efr::SampleIdentity toSampleId(Guid const& i_id);

Guid toLetsTalkGuid(efr::SampleIdentity const& i_sampleId);

std::ostream& operator<<(std::ostream& os, Guid const& i_guid);
}  // namespace lt