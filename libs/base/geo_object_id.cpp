#include "base/geo_object_id.hpp"

#include "base/assert.hpp"

#include <sstream>

namespace
{
// todo(@m) Uncomment when the transition from osm::Id to base::GeoObjectId is complete
//          and add assertions about the highest bit.
//          The old scheme used the highest bit and the new one does not.
// uint64_t const kTypeMask = 0x7F00000000000000;
uint64_t const kTypeMask = 0xFF00000000000000;
uint64_t const kReservedMask = 0x00FF000000000000;
uint64_t const kSerialMask = 0x0000FFFFFFFFFFFF;
}  // namespace

namespace base
{
GeoObjectId::GeoObjectId(uint64_t encodedId) : m_encodedId(encodedId) {}

GeoObjectId::GeoObjectId(Type type, uint64_t id) : m_encodedId((static_cast<uint64_t>(type) << 56) | id) {}

uint64_t GeoObjectId::GetSerialId() const
{
  ASSERT_NOT_EQUAL(m_encodedId & kTypeMask, 0, ());
  ASSERT_EQUAL(m_encodedId & kReservedMask, 0, ());
  return m_encodedId & kSerialMask;
}

uint64_t GeoObjectId::GetEncodedId() const
{
  return m_encodedId;
}

GeoObjectId::Type GeoObjectId::GetType() const
{
  ASSERT_EQUAL(m_encodedId & kReservedMask, 0, ());
  uint64_t const typeBits = (m_encodedId & kTypeMask) >> 56;
  switch (typeBits)
  {
  case 0x00: return Type::Invalid;
  case 0x01: return Type::OsmNode;
  case 0x02: return Type::OsmWay;
  case 0x03: return Type::OsmRelation;
  case 0x04: return Type::BookingComNode;
  case 0x05: return Type::OsmSurrogate;
  case 0x06: return Type::Fias;
  case 0x40: return Type::ObsoleteOsmNode;
  case 0x80: return Type::ObsoleteOsmWay;
  case 0xC0: return Type::ObsoleteOsmRelation;
  }
  UNREACHABLE();
}

GeoObjectId MakeOsmNode(uint64_t id)
{
  return GeoObjectId(GeoObjectId::Type::ObsoleteOsmNode, id);
}

GeoObjectId MakeOsmWay(uint64_t id)
{
  return GeoObjectId(GeoObjectId::Type::ObsoleteOsmWay, id);
}

GeoObjectId MakeOsmRelation(uint64_t id)
{
  return GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, id);
}

std::ostream & operator<<(std::ostream & os, GeoObjectId const & geoObjectId)
{
  os << geoObjectId.GetEncodedId();
  return os;
}

std::istream & operator>>(std::istream & os, GeoObjectId & geoObjectId)
{
  uint64_t encodedId;
  os >> encodedId;
  geoObjectId = GeoObjectId(encodedId);
  return os;
}

std::string DebugPrint(GeoObjectId::Type const & t)
{
  switch (t)
  {
  case GeoObjectId::Type::Invalid: return "Invalid";
  case GeoObjectId::Type::OsmNode: return "Osm Node";
  case GeoObjectId::Type::OsmWay: return "Osm Way";
  case GeoObjectId::Type::OsmRelation: return "Osm Relation";
  case GeoObjectId::Type::BookingComNode: return "Booking.com";
  case GeoObjectId::Type::OsmSurrogate: return "Osm Surrogate";
  case GeoObjectId::Type::Fias: return "FIAS";
  case GeoObjectId::Type::ObsoleteOsmNode: return "Osm Node";
  case GeoObjectId::Type::ObsoleteOsmWay: return "Osm Way";
  case GeoObjectId::Type::ObsoleteOsmRelation: return "Osm Relation";
  }
  UNREACHABLE();
}

std::string DebugPrint(GeoObjectId const & id)
{
  std::ostringstream oss;
  // GetSerialId() does not work for invalid ids but we may still want to print them.
  oss << DebugPrint(id.GetType()) << " " << (id.GetEncodedId() & kSerialMask);
  return oss.str();
}
}  // namespace base
