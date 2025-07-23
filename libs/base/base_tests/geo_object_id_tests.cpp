#include "testing/testing.hpp"

#include "base/geo_object_id.hpp"

#include <iostream>

using namespace base;

UNIT_TEST(GeoObjectId_Smoke)
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

UNIT_TEST(GeoObjectId_Print)
{
  // https://www.openstreetmap.org/#map=19/54.54438/25.69932

  // Belarus -> Lithuania                                  B          L
  LOG(LINFO, (MakeOsmWay(533044131).GetEncodedId()));  // 1 3 0
  LOG(LINFO, (MakeOsmWay(489294139).GetEncodedId()));  //            1 0 1

  // Lithuania -> Belarus
  LOG(LINFO, (MakeOsmWay(614091318).GetEncodedId()));  // 1 5 1      1 5 0
  LOG(LINFO, (MakeOsmWay(148247387).GetEncodedId()));

  // https://www.openstreetmap.org/#map=19/55.30215/10.86668

  // Denmark_Zealand -> Denmark_Southern                 Z            S
  LOG(LINFO, (MakeOsmWay(2032610).GetEncodedId()));  // 1 6 0        1 6 1

  // Denmark_Southern -> Denmark_Zealand
  LOG(LINFO, (MakeOsmWay(4360600).GetEncodedId()));  // 1 18 1       1 18 0
}
