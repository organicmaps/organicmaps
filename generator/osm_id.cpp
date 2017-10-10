#include "generator/osm_id.hpp"

#include "base/assert.hpp"

#include <sstream>


namespace osm
{

// Use 3 higher bits to encode type
static const uint64_t NODE = 0x4000000000000000ULL;
static const uint64_t WAY = 0x8000000000000000ULL;
static const uint64_t RELATION = 0xC000000000000000ULL;
static const uint64_t RESET = ~(NODE | WAY | RELATION);

Id::Id(uint64_t encodedId) : m_encodedId(encodedId)
{
}

Id Id::Node(uint64_t id)
{
  return Id( id | NODE );
}

Id Id::Way(uint64_t id)
{
  return Id( id | WAY );
}

Id Id::Relation(uint64_t id)
{
  return Id( id | RELATION );
}

uint64_t Id::OsmId() const
{
  return m_encodedId & RESET;
}

uint64_t Id::EncodedId() const
{
  return m_encodedId;
}

bool Id::IsNode() const
{
  return ((m_encodedId & NODE) == NODE);
}

bool Id::IsWay() const
{
  return ((m_encodedId & WAY) == WAY);
}

bool Id::IsRelation() const
{
  return ((m_encodedId & RELATION) == RELATION);
}

std::string Id::Type() const
{
  if ((m_encodedId & RELATION) == RELATION)
    return "relation";
  else if ((m_encodedId & NODE) == NODE)
    return "node";
  else if ((m_encodedId & WAY) == WAY)
    return "way";
  else
    return "ERROR: Not initialized Osm ID";
}

std::string DebugPrint(osm::Id const & id)
{
  std::ostringstream stream;
  stream << id.Type() << " " << id.OsmId();
  return stream.str();
}

}  // namespace osm
