#include "base/osm_id.hpp"

#include "base/assert.hpp"

#include <sstream>

namespace
{
// Use 2 higher bits to encode type.
// Note that the masks are not disjoint.
uint64_t const kNode = 0x4000000000000000ULL;
uint64_t const kWay = 0x8000000000000000ULL;
uint64_t const kRelation = 0xC000000000000000ULL;
uint64_t const kTypeMask = 0xC000000000000000ULL;
}  // namespace

namespace osm
{
Id::Id(uint64_t encodedId) : m_encodedId(encodedId)
{
}

Id Id::Node(uint64_t id)
{
  ASSERT_EQUAL(id & kTypeMask, 0, ());
  return Id(id | kNode);
}

Id Id::Way(uint64_t id)
{
  ASSERT_EQUAL(id & kTypeMask, 0, ());
  return Id(id | kWay);
}

Id Id::Relation(uint64_t id)
{
  ASSERT_EQUAL(id & kTypeMask, 0, ());
  return Id(id | kRelation);
}

uint64_t Id::GetOsmId() const
{
  ASSERT_NOT_EQUAL(m_encodedId & kTypeMask, 0, ());
  return m_encodedId & ~kTypeMask;
}

uint64_t Id::GetEncodedId() const
{
  return m_encodedId;
}

Id::Type Id::GetType() const
{
  uint64_t const mask = m_encodedId & kTypeMask;
  switch (mask)
  {
  case kNode: return Id::Type::Node;
  case kWay: return Id::Type::Way;
  case kRelation: return Id::Type::Relation;
  }
  CHECK_SWITCH();
}

std::string DebugPrint(Id::Type const & t)
{
  switch (t)
  {
  case Id::Type::Node: return "node";
  case Id::Type::Way: return "way";
  case Id::Type::Relation: return "relation";
  }
  CHECK_SWITCH();
}

std::string DebugPrint(Id const & id)
{
  std::ostringstream oss;
  oss << DebugPrint(id.GetType()) << " " << id.GetOsmId();
  return oss.str();
}
}  // namespace osm
