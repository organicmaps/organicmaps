#pragma once

#include <cstdint>
#include <functional>
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
  uint64_t EncodedId() const;
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

struct HashId : private std::hash<uint64_t>
{
  size_t operator()(Id const & id) const { return std::hash<uint64_t>::operator()(id.OsmId()); }
};

std::string DebugPrint(osm::Id const & id);
}  // namespace osm
