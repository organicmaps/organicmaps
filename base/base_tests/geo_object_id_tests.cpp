#include "testing/testing.hpp"

#include "base/geo_object_id.hpp"

namespace base
{
UNIT_TEST(GeoObjectId)
{
  GeoObjectId const invalid(GeoObjectId::kInvalid);
  TEST_EQUAL(invalid.GetType(), GeoObjectId::Type::Invalid, ());
  TEST_EQUAL(DebugPrint(invalid), "Invalid 0", ());

  GeoObjectId const node(GeoObjectId::Type::ObsoleteOsmNode, 12345);
  TEST_EQUAL(node.GetSerialId(), 12345ULL, ());
  TEST_EQUAL(node.GetType(), GeoObjectId::Type::ObsoleteOsmNode, ());
  TEST_EQUAL(DebugPrint(node), "Osm Node 12345", ());

  GeoObjectId const way(GeoObjectId::Type::ObsoleteOsmWay, 93245123456332ULL);
  TEST_EQUAL(way.GetSerialId(), 93245123456332ULL, ());
  TEST_EQUAL(way.GetType(), GeoObjectId::Type::ObsoleteOsmWay, ());
  TEST_EQUAL(DebugPrint(way), "Osm Way 93245123456332", ());

  GeoObjectId const relation(GeoObjectId::Type::ObsoleteOsmRelation, 5);
  TEST_EQUAL(relation.GetSerialId(), 5ULL, ());
  TEST_EQUAL(relation.GetType(), GeoObjectId::Type::ObsoleteOsmRelation, ());
  TEST_EQUAL(DebugPrint(relation), "Osm Relation 5", ());

  // 2^48 - 1, maximal possible serial id.
  GeoObjectId const surrogate(GeoObjectId::Type::OsmSurrogate, 281474976710655ULL);
  TEST_EQUAL(surrogate.GetSerialId(), 281474976710655ULL, ());
  TEST_EQUAL(surrogate.GetType(), GeoObjectId::Type::OsmSurrogate, ());
  TEST_EQUAL(DebugPrint(surrogate), "Osm Surrogate 281474976710655", ());

  // 0 is not prohibited by the encoding even though OSM ids start from 1.
  GeoObjectId const booking(GeoObjectId::Type::BookingComNode, 0);
  TEST_EQUAL(booking.GetSerialId(), 0, ());
  TEST_EQUAL(booking.GetType(), GeoObjectId::Type::BookingComNode, ());
  TEST_EQUAL(DebugPrint(booking), "Booking.com 0", ());

  GeoObjectId const fias(GeoObjectId::Type::Fias, 0xf1a5);
  TEST_EQUAL(fias.GetSerialId(), 0xf1a5, ());
  TEST_EQUAL(fias.GetType(), GeoObjectId::Type::Fias, ());
  TEST_EQUAL(DebugPrint(fias), "FIAS 61861", ());
}
}  // namespace base
