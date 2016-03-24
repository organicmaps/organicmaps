#pragma once

#include "std/cstdint.hpp"
#include "std/string.hpp"


namespace osm
{

class Id
{
  uint64_t m_encodedId;

  static const uint64_t INVALID = 0ULL;

public:
  explicit Id(uint64_t encodedId = INVALID);

  static Id Node(uint64_t osmId);
  static Id Way(uint64_t osmId);
  static Id Relation(uint64_t osmId);

  uint64_t OsmId() const;
  bool IsWay() const;

  /// For debug output
  string Type() const;

  bool operator<(Id const & other) const { return m_encodedId < other.m_encodedId; }
  bool operator==(Id const & other) const { return m_encodedId == other.m_encodedId; }
  bool operator==(uint64_t other) const { return OsmId() == other; }
};

string DebugPrint(osm::Id const & id);

} // namespace osm
