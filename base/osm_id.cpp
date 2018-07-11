#include "base/osm_id.hpp"

#include <sstream>

namespace
{
// Use 2 higher bits to encode type.
//
// todo The masks are not disjoint for some reason
//      since the commit 60414dc86254aed22ac9e66fed49eba554260a2c.
uint64_t const kNode = 0x4000000000000000ULL;
uint64_t const kWay = 0x8000000000000000ULL;
uint64_t const kRelation = 0xC000000000000000ULL;
uint64_t const kReset = ~(kNode | kWay | kRelation);
}  // namespace

namespace osm
{
Id::Id(uint64_t encodedId) : m_encodedId(encodedId)
{
}

Id Id::Node(uint64_t id)
{
  return Id(id | kNode);
}

Id Id::Way(uint64_t id)
{
  return Id(id | kWay);
}

Id Id::Relation(uint64_t id)
{
  return Id(id | kRelation);
}

uint64_t Id::OsmId() const
{
  return m_encodedId & kReset;
}

uint64_t Id::EncodedId() const
{
  return m_encodedId;
}

bool Id::IsNode() const
{
  return (m_encodedId & kNode) == kNode;
}

bool Id::IsWay() const
{
  return (m_encodedId & kWay) == kWay;
}

bool Id::IsRelation() const
{
  return (m_encodedId & kRelation) == kRelation;
}

std::string DebugPrint(osm::Id const & id)
{
  std::string typeStr;
  // Note that with current encoding all relations are also at the
  // same time nodes and ways. Therefore, the relation check must go first.
  if (id.IsRelation())
    typeStr = "relation";
  else if (id.IsNode())
    typeStr = "node";
  else if (id.IsWay())
    typeStr = "way";
  else
    typeStr = "ERROR: Not initialized Osm ID";

  std::ostringstream stream;
  stream << typeStr << " " << id.OsmId();
  return stream.str();
}
}  // namespace osm
