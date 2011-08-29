#include "osm_id.hpp"

#include "../base/assert.hpp"

namespace osm
{

// Use 3 higher bits to encode type
static const uint64_t NODE = 0x2000000000000000ULL;
static const uint64_t WAY = 0x4000000000000000ULL;
static const uint64_t RELATION = 0x8000000000000000ULL;
static const uint64_t RESET = ~(NODE | WAY | RELATION);

OsmId::OsmId(string const & type, uint64_t osmId)
  : m_id(osmId)
{
  if (type == "node")
    m_id |= NODE;
  else if (type == "way")
    m_id |= WAY;
  else
  {
    m_id |= RELATION;
    ASSERT_EQUAL(type, "relation", ("Invalid osm type:", type));
  }
}

uint64_t OsmId::Id() const
{
  return m_id & RESET;
}

string OsmId::Type() const
{
  if (m_id & NODE)
    return "node";
  else if (m_id & WAY)
    return "way";
  else
    return "relation";
}

}  // namespace osm
