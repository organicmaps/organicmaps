#pragma once

#include <cstdint>
#include <string>


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
  bool IsNode() const;
  bool IsWay() const;
  bool IsRelation() const;

  /// For debug output
  std::string Type() const;

  inline bool operator<(Id const & other) const { return m_encodedId < other.m_encodedId; }
  inline bool operator==(Id const & other) const { return m_encodedId == other.m_encodedId; }
  inline bool operator!=(Id const & other) const { return !(*this == other); }
  bool operator==(uint64_t other) const { return OsmId() == other; }
};

std::string DebugPrint(osm::Id const & id);
} // namespace osm
