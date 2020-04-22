#include "testing/testing.hpp"

#include "routing/road_access.hpp"
#include "routing/road_access_serialization.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "3party/opening_hours/opening_hours.hpp"

using namespace routing;
using namespace routing_test;
using namespace std;

using TestEdge = TestIndexGraphTopology::Edge;

namespace
{
void FillRoadAccessBySample_1(RoadAccess & roadAccess)
{
  RoadAccess::WayToAccess wayToAccess = {
      {1 /* featureId */, RoadAccess::Type::No},
      {2 /* featureId */, RoadAccess::Type::Private},
  };

  RoadAccess::PointToAccess pointToAccess = {
      {RoadPoint(3 /* featureId */, 0 /* pointId */), RoadAccess::Type::No},
      {RoadPoint(4 /* featureId */, 7 /* pointId */), RoadAccess::Type::Private},
  };

  roadAccess.SetAccess(move(wayToAccess), move(pointToAccess));
}

void FillRoadAccessBySample_2(RoadAccess & roadAccess)
{
  RoadAccess::WayToAccess wayToAccess = {
      {1 /* featureId */, RoadAccess::Type::Private},
      {2 /* featureId */, RoadAccess::Type::Destination},
  };

  RoadAccess::PointToAccess pointToAccess = {
      {RoadPoint(3 /* featureId */, 10 /* pointId */), RoadAccess::Type::Destination},
      {RoadPoint(4 /* featureId */, 0 /* pointId */), RoadAccess::Type::No},
  };

  roadAccess.SetAccess(move(wayToAccess), move(pointToAccess));
}

//void FillRoadAccessBySampleConditional_1(RoadAccess & roadAccess)
//{
//  std::vector<std::string> const openingHoursStrings = {
//      "Mo-Su", "10:00-18:00", "Mo-Fr 10:00-14:00", "09:00-13:00", "Apr - May", "2010 - 2100"};
//
//  std::vector<osmoh::OpeningHours> openingHours;
//  for (auto const & oh : openingHoursStrings)
//  {
//    openingHours.emplace_back(oh);
//    TEST(openingHours.back().IsValid(), ());
//  }
//
//  RoadAccess::Conditional conditional_1;
//  conditional_1.Insert(RoadAccess::Type::No, move(openingHours[0]));
//  conditional_1.Insert(RoadAccess::Type::Private, move(openingHours[1]));
//
//  RoadAccess::Conditional conditional_2;
//  conditional_2.Insert(RoadAccess::Type::Destination, move(openingHours[2]));
//
//  RoadAccess::Conditional conditional_3;
//  conditional_3.Insert(RoadAccess::Type::No, move(openingHours[4]));
//  conditional_3.Insert(RoadAccess::Type::Destination, move(openingHours[3]));
//
//  RoadAccess::Conditional conditional_4;
//  conditional_4.Insert(RoadAccess::Type::Destination, move(openingHours[5]));
//
//  RoadAccess::WayToAccessConditional wayToAccessConditional = {{1 /* featureId */, conditional_1},
//                                                               {2 /* featureId */, conditional_2}};
//
//  RoadAccess::PointToAccessConditional pointToAccessConditional = {
//      {RoadPoint(3 /* featureId */, 0 /* pointId */), conditional_3},
//      {RoadPoint(4 /* featureId */, 7 /* pointId */), conditional_4}};
//
//  roadAccess.SetAccessConditional(move(wayToAccessConditional), move(pointToAccessConditional));
//}
//
//void FillRoadAccessBySampleConditional_2(RoadAccess & roadAccess)
//{
//  std::vector<std::string> const openingHoursStrings = {
//      "Mo", "Apr-May 03:00-04:25", "Mo-Sa 12:00-13:00", "2010-2098", "Nov-Apr", "19:00-21:00"};
//
//  std::vector<osmoh::OpeningHours> openingHours;
//  for (auto const & oh : openingHoursStrings)
//  {
//    openingHours.emplace_back(oh);
//    TEST(openingHours.back().IsValid(), (oh));
//  }
//
//  RoadAccess::Conditional conditional_1;
//  conditional_1.Insert(RoadAccess::Type::Private, move(openingHours[0]));
//
//  RoadAccess::Conditional conditional_2;
//  conditional_2.Insert(RoadAccess::Type::No, move(openingHours[1]));
//  conditional_2.Insert(RoadAccess::Type::Private, move(openingHours[2]));
//
//  RoadAccess::Conditional conditional_3;
//  conditional_3.Insert(RoadAccess::Type::Destination, move(openingHours[3]));
//
//  RoadAccess::Conditional conditional_4;
//  conditional_4.Insert(RoadAccess::Type::No, move(openingHours[4]));
//  conditional_4.Insert(RoadAccess::Type::No, move(openingHours[5]));
//
//  RoadAccess::WayToAccessConditional wayToAccessConditional = {{1 /* featureId */, conditional_1},
//                                                               {2 /* featureId */, conditional_2}};
//
//  RoadAccess::PointToAccessConditional pointToAccessConditional = {
//      {RoadPoint(3 /* featureId */, 10 /* pointId */), conditional_3},
//      {RoadPoint(4 /* featureId */, 2 /* pointId */), conditional_4}};
//
//  roadAccess.SetAccessConditional(move(wayToAccessConditional), move(pointToAccessConditional));
//}

class RoadAccessSerDesTest
{
public:
  void Serialize(RoadAccessSerializer::RoadAccessByVehicleType const & roadAccessAllTypes)
  {
    MemWriter writer(m_buffer);
    RoadAccessSerializer::Serialize(writer, roadAccessAllTypes);
  }

  void TestDeserialize(VehicleType vehicleType, RoadAccess const & answer)
  {
    RoadAccess deserializedRoadAccess;

    MemReader memReader(m_buffer.data(), m_buffer.size());
    ReaderSource<MemReader> src(memReader);
    RoadAccessSerializer::Deserialize(src, vehicleType, deserializedRoadAccess, string("unknown"));
    TEST_EQUAL(answer, deserializedRoadAccess, ());
  }

  void ClearBuffer() { m_buffer.clear(); }

private:
  vector<uint8_t> m_buffer;
};

UNIT_CLASS_TEST(RoadAccessSerDesTest, RoadAccess_Serdes)
{
  RoadAccess roadAccessCar;
  FillRoadAccessBySample_1(roadAccessCar);

  RoadAccess roadAccessPedestrian;
  FillRoadAccessBySample_2(roadAccessPedestrian);

  RoadAccessSerializer::RoadAccessByVehicleType roadAccessAllTypes;
  roadAccessAllTypes[static_cast<size_t>(VehicleType::Car)] = roadAccessCar;
  roadAccessAllTypes[static_cast<size_t>(VehicleType::Pedestrian)] = roadAccessPedestrian;

  Serialize(roadAccessAllTypes);
  TestDeserialize(VehicleType::Car, roadAccessCar);
  TestDeserialize(VehicleType::Pedestrian, roadAccessPedestrian);
}

// @TODO Tests below and functions FillRoadAccessBySampleConditional_1 and
// FillRoadAccessBySampleConditional_2 should be uncommented when access:conditional is
// switched on.
//UNIT_CLASS_TEST(RoadAccessSerDesTest, RoadAccess_Serdes_Conditional_One_Vehicle)
//{
//  auto constexpr kVehicleTypeCount = static_cast<size_t>(VehicleType::Count);
//  for (size_t vehicleTypeId = 0; vehicleTypeId < kVehicleTypeCount; ++vehicleTypeId)
//  {
//    RoadAccess roadAccess;
//    FillRoadAccessBySampleConditional_1(roadAccess);
//
//    RoadAccessSerializer::RoadAccessByVehicleType roadAccessAllTypes;
//    roadAccessAllTypes[vehicleTypeId] = roadAccess;
//
//    Serialize(roadAccessAllTypes);
//    TestDeserialize(static_cast<VehicleType>(vehicleTypeId), roadAccess);
//    ClearBuffer();
//  }
//}
//
//UNIT_CLASS_TEST(RoadAccessSerDesTest, RoadAccess_Serdes_Conditional_Several_Vehicles)
//{
//  RoadAccess roadAccessCar;
//  FillRoadAccessBySampleConditional_1(roadAccessCar);
//
//  RoadAccess roadAccessPedestrian;
//  FillRoadAccessBySampleConditional_2(roadAccessPedestrian);
//
//  RoadAccessSerializer::RoadAccessByVehicleType roadAccessAllTypes;
//  roadAccessAllTypes[static_cast<size_t>(VehicleType::Car)] = roadAccessCar;
//  roadAccessAllTypes[static_cast<size_t>(VehicleType::Pedestrian)] = roadAccessPedestrian;
//
//  Serialize(roadAccessAllTypes);
//  TestDeserialize(VehicleType::Car, roadAccessCar);
//  TestDeserialize(VehicleType::Pedestrian, roadAccessPedestrian);
//}
//
//UNIT_CLASS_TEST(RoadAccessSerDesTest, RoadAccess_Serdes_Conditional_Mixed_Several_Vehicles)
//{
//  RoadAccess roadAccessCar;
//  FillRoadAccessBySampleConditional_1(roadAccessCar);
//  FillRoadAccessBySample_1(roadAccessCar);
//
//  RoadAccess roadAccessPedestrian;
//  FillRoadAccessBySampleConditional_2(roadAccessPedestrian);
//  FillRoadAccessBySample_2(roadAccessPedestrian);
//
//  RoadAccessSerializer::RoadAccessByVehicleType roadAccessAllTypes;
//  roadAccessAllTypes[static_cast<size_t>(VehicleType::Car)] = roadAccessCar;
//  roadAccessAllTypes[static_cast<size_t>(VehicleType::Pedestrian)] = roadAccessPedestrian;
//
//  Serialize(roadAccessAllTypes);
//  TestDeserialize(VehicleType::Car, roadAccessCar);
//  TestDeserialize(VehicleType::Pedestrian, roadAccessPedestrian);
//}

UNIT_TEST(RoadAccess_WayBlocked)
{
  // Add edges to the graph in the following format: (from, to, weight).
  // Block edges in the following format: (from, to).

  uint32_t const numVertices = 4;
  TestIndexGraphTopology graph(numVertices);

  graph.AddDirectedEdge(0, 1, 1.0);
  graph.AddDirectedEdge(1, 2, 1.0);
  graph.AddDirectedEdge(2, 3, 1.0);

  double const expectedWeight = 0.0;
  vector<TestEdge> const expectedEdges = {};

  graph.SetEdgeAccess(1, 2, RoadAccess::Type::No);

  TestTopologyGraph(graph, 0, 3, false /* pathFound */, expectedWeight, expectedEdges);
}

UNIT_TEST(RoadAccess_WayBlocking)
{
  uint32_t const numVertices = 6;
  TestIndexGraphTopology graph(numVertices);

  graph.AddDirectedEdge(0, 1, 1.0);
  graph.AddDirectedEdge(1, 2, 1.0);
  graph.AddDirectedEdge(2, 5, 1.0);
  graph.AddDirectedEdge(0, 3, 1.0);
  graph.AddDirectedEdge(3, 4, 2.0);
  graph.AddDirectedEdge(4, 5, 1.0);

  double expectedWeight;
  vector<TestEdge> expectedEdges;

  expectedWeight = 3.0;
  expectedEdges = {{0, 1}, {1, 2}, {2, 5}};
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test avoid access=private while we have route with RoadAccess::Type::Yes.
  graph.SetEdgeAccess(1, 2, RoadAccess::Type::Private);
  expectedWeight = 4.0;
  expectedEdges = {{0, 3}, {3, 4}, {4, 5}};
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test avoid access=destination while we have route with RoadAccess::Type::Yes.
  graph.SetEdgeAccess(1, 2, RoadAccess::Type::Destination);
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test avoid access=no while we have route with RoadAccess::Type::Yes.
  graph.SetEdgeAccess(1, 2, RoadAccess::Type::No);
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test it's possible to build the route because private usage restriction is not strict:
  // we use minimal possible number of access=yes/access={private, destination} crossing
  // but allow to use private/destination if there is no other way.
  graph.SetEdgeAccess(3, 4, RoadAccess::Type::Private);
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test we have the same behaviour for access=destination.
  graph.SetEdgeAccess(3, 4, RoadAccess::Type::Destination);
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test we have strict restriction for access=no and can not build route.
  graph.SetEdgeAccess(3, 4, RoadAccess::Type::No);
  TestTopologyGraph(graph, 0, 5, false /* pathFound */, expectedWeight, expectedEdges);
}

UNIT_TEST(RoadAccess_BarrierBypassing)
{
  uint32_t const numVertices = 6;
  TestIndexGraphTopology graph(numVertices);

  graph.AddDirectedEdge(0, 1, 1.0);
  graph.AddDirectedEdge(1, 2, 1.0);
  graph.AddDirectedEdge(2, 5, 1.0);
  graph.AddDirectedEdge(0, 3, 1.0);
  graph.AddDirectedEdge(3, 4, 2.0);
  graph.AddDirectedEdge(4, 5, 1.0);

  double expectedWeight;
  vector<TestEdge> expectedEdges;

  expectedWeight = 3.0;
  expectedEdges = {{0, 1}, {1, 2}, {2, 5}};
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test avoid access=private while we have route with RoadAccess::Type::Yes.
  graph.SetVertexAccess(1, RoadAccess::Type::Private);
  expectedWeight = 4.0;
  expectedEdges = {{0, 3}, {3, 4}, {4, 5}};
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test avoid access=destination while we have route with RoadAccess::Type::Yes.
  graph.SetVertexAccess(1, RoadAccess::Type::Destination);
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test avoid access=no while we have route with RoadAccess::Type::Yes.
  graph.SetVertexAccess(1, RoadAccess::Type::No);
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test it's possible to build the route because private usage restriction is not strict:
  // we use minimal possible number of access=yes/access={private, destination} crossing
  // but allow to use private/destination if there is no other way.
  graph.SetVertexAccess(3, RoadAccess::Type::Private);
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test we have the same behaviour for access=destination.
  graph.SetVertexAccess(3, RoadAccess::Type::Destination);
  TestTopologyGraph(graph, 0, 5, true /* pathFound */, expectedWeight, expectedEdges);

  // Test we have strict restriction for access=no and can not build route.
  graph.SetVertexAccess(3, RoadAccess::Type::No);
  TestTopologyGraph(graph, 0, 5, false /* pathFound */, expectedWeight, expectedEdges);
}

UNIT_TEST(RoadAccess_WayBlockedConditional)
{
  uint32_t const numVertices = 4;
  TestIndexGraphTopology graph(numVertices);

  graph.AddDirectedEdge(0, 1, 1.0);
  graph.AddDirectedEdge(1, 2, 1.0);
  graph.AddDirectedEdge(2, 3, 1.0);

  double expectedWeight = 3.0;
  vector<TestEdge> expectedEdges = {{0, 1}, {1, 2}, {2, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  graph.SetEdgeAccessConditional(1, 2, RoadAccess::Type::No, "Jan - Jul");

  auto const april = []() {
    return GetUnixtimeByDate(2020, Month::Apr, 1, 12 /* hh */, 00 /* mm */);
  };

  graph.SetCurrentTimeGetter(april);
  expectedWeight = 0;
  expectedEdges = {};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, false /* pathFound */, expectedWeight,
                    expectedEdges);

  auto const november = []() {
    return GetUnixtimeByDate(2020, Month::Nov, 1, 12 /* hh */, 00 /* mm */);
  };

  graph.SetCurrentTimeGetter(november);
  expectedWeight = 3.0;
  expectedEdges = {{0, 1}, {1, 2}, {2, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);
}

//UNIT_TEST(RoadAccess_PointBlockedConditional)
//{
//  uint32_t const numVertices = 4;
//  TestIndexGraphTopology graph(numVertices);
//
//  graph.AddDirectedEdge(0, 1, 1.0);
//  graph.AddDirectedEdge(1, 2, 1.0);
//  graph.AddDirectedEdge(2, 3, 1.0);
//
//  double expectedWeight = 3.0;
//  vector<TestEdge> expectedEdges = {{0, 1}, {1, 2}, {2, 3}};
//  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
//                    expectedEdges);
//
//  graph.SetVertexAccessConditional(1, RoadAccess::Type::No, "Jan - Jul");
//
//  auto const april = []() {
//    return GetUnixtimeByDate(2020, Month::Apr, 1, 12 /* hh */, 00 /* mm */);
//  };
//
//  graph.SetCurrentTimeGetter(april);
//  expectedWeight = 0;
//  expectedEdges = {};
//  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, false /* pathFound */, expectedWeight,
//                    expectedEdges);
//
//  auto const november = []() {
//    return GetUnixtimeByDate(2020, Month::Nov, 1, 12 /* hh */, 00 /* mm */);
//  };
//
//  graph.SetCurrentTimeGetter(november);
//  expectedWeight = 3.0;
//  expectedEdges = {{0, 1}, {1, 2}, {2, 3}};
//  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
//                    expectedEdges);
//}

UNIT_TEST(RoadAccess_WayBlockedAvoidConditional)
{
  uint32_t const numVertices = 4;
  TestIndexGraphTopology graph(numVertices);

  graph.AddDirectedEdge(0, 1, 1.0);
  graph.AddDirectedEdge(0, 2, 10.0);
  graph.AddDirectedEdge(1, 3, 1.0);
  graph.AddDirectedEdge(2, 3, 10.0);

  double expectedWeight = 2.0;
  vector<TestEdge> expectedEdges = {{0, 1}, {1, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  graph.SetEdgeAccessConditional(0, 1, RoadAccess::Type::No, "Mo-Fr 10:00 - 19:00");

  auto const mondayAlmostTenHours = []() {
    return GetUnixtimeByDate(2020, Month::Apr, Weekday::Monday, 9 /* hh */, 50 /* mm */);
  };

  // In this time we probably will able to pass 0->1 edge, but we are not sure, so we should avoid
  // such edges.
  graph.SetCurrentTimeGetter(mondayAlmostTenHours);
  expectedWeight = 20.0;
  expectedEdges = {{0, 2}, {2, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  graph.SetEdgeAccess(0, 2, RoadAccess::Type::No);

  // But if this is the only path (we blocked 0->2 above), we should pass edge 0->1 anyway.
  graph.SetCurrentTimeGetter(mondayAlmostTenHours);
  expectedWeight = 2.0;
  expectedEdges = {{0, 1}, {1, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  auto const mondayTwelveHours = []() {
    return GetUnixtimeByDate(2020, Month::Apr, Weekday::Monday, 12 /* hh */, 00 /* mm */);
  };

  // But if we sure that in this time edge: 0->1 will be blocked, we definitely should not pass
  // 0->1. In this case no way will be found.
  graph.SetCurrentTimeGetter(mondayTwelveHours);
  expectedWeight = 0.0;
  expectedEdges = {};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, false /* pathFound */, expectedWeight,
                    expectedEdges);
}

//UNIT_TEST(RoadAccess_PointBlockedAvoidConditional)
//{
//  uint32_t const numVertices = 4;
//  TestIndexGraphTopology graph(numVertices);
//
//  graph.AddDirectedEdge(0, 1, 1.0);
//  graph.AddDirectedEdge(0, 2, 10.0);
//  graph.AddDirectedEdge(1, 3, 1.0);
//  graph.AddDirectedEdge(2, 3, 10.0);
//
//  double expectedWeight = 2.0;
//  vector<TestEdge> expectedEdges = {{0, 1}, {1, 3}};
//  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
//                    expectedEdges);
//
//  graph.SetVertexAccessConditional(1, RoadAccess::Type::No, "Mo-Fr 10:00 - 19:00");
//
//  auto const mondayAlmostTenHours = []() {
//    return GetUnixtimeByDate(2020, Month::Apr, Weekday::Monday, 9 /* hh */, 50 /* mm */);
//  };
//
//  // In this time we probably will able to pass vertex: 1, but we are not sure, so we should avoid
//  // such edges.
//  graph.SetCurrentTimeGetter(mondayAlmostTenHours);
//  expectedWeight = 20.0;
//  expectedEdges = {{0, 2}, {2, 3}};
//  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
//                    expectedEdges);
//
//  graph.SetEdgeAccess(0, 2, RoadAccess::Type::No);
//
//  // But if this is the only path (we blocked 0->2 above), we should pass through vertex: 1 anyway.
//  graph.SetCurrentTimeGetter(mondayAlmostTenHours);
//  expectedWeight = 2.0;
//  expectedEdges = {{0, 1}, {1, 3}};
//  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
//                    expectedEdges);
//
//  auto const mondayTwelveHours = []() {
//    return GetUnixtimeByDate(2020, Month::Apr, Weekday::Monday, 12 /* hh */, 00 /* mm */);
//  };
//
//  // But if we sure that in this time vertex: 1 will be blocked, we definitely should not pass
//  // through vertex: 1. In this case no way will be found.
//  graph.SetCurrentTimeGetter(mondayTwelveHours);
//  expectedWeight = 0.0;
//  expectedEdges = {};
//  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, false /* pathFound */, expectedWeight,
//                    expectedEdges);
//}

UNIT_TEST(RoadAccess_WayBlockedConditional_Yes_No)
{
  uint32_t const numVertices = 4;
  TestIndexGraphTopology graph(numVertices);

  graph.AddDirectedEdge(0, 1, 1.0);
  graph.AddDirectedEdge(1, 2, 1.0);
  graph.AddDirectedEdge(2, 3, 1.0);

  double expectedWeight = 3.0;
  vector<TestEdge> expectedEdges = {{0, 1}, {1, 2}, {2, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  graph.SetEdgeAccessConditional(1, 2, RoadAccess::Type::No, "Mo-Fr");
  graph.SetEdgeAccessConditional(1, 2, RoadAccess::Type::Yes, "Sa-Su");

  auto const tuesday = []() {
    return GetUnixtimeByDate(2020, Month::Apr, Weekday::Tuesday, 10 /* hh */, 00 /* mm */);
  };

  // Way is blocked from Monday to Friday
  graph.SetCurrentTimeGetter(tuesday);
  expectedWeight = 0;
  expectedEdges = {};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, false /* pathFound */, expectedWeight,
                    expectedEdges);

  auto const saturday = []() {
    return GetUnixtimeByDate(2020, Month::Nov, Weekday::Saturday, 10 /* hh */, 00 /* mm */);
  };

  // And open from Saturday to Sunday
  graph.SetCurrentTimeGetter(saturday);
  expectedWeight = 3.0;
  expectedEdges = {{0, 1}, {1, 2}, {2, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);
}

//UNIT_TEST(RoadAccess_PointBlockedConditional_Yes_No)
//{
//  uint32_t const numVertices = 4;
//  TestIndexGraphTopology graph(numVertices);
//
//  graph.AddDirectedEdge(0, 1, 1.0);
//  graph.AddDirectedEdge(1, 2, 1.0);
//  graph.AddDirectedEdge(2, 3, 1.0);
//
//  double expectedWeight = 3.0;
//  vector<TestEdge> expectedEdges = {{0, 1}, {1, 2}, {2, 3}};
//  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
//                    expectedEdges);
//
//  graph.SetVertexAccessConditional(1, RoadAccess::Type::No, "Mo-Fr");
//  graph.SetVertexAccessConditional(1, RoadAccess::Type::Yes, "Sa-Su");
//
//  auto const tuesday = []() {
//    return GetUnixtimeByDate(2020, Month::Apr, Weekday::Tuesday, 10 /* hh */, 00 /* mm */);
//  };
//
//  // Way is blocked from Monday to Friday
//  graph.SetCurrentTimeGetter(tuesday);
//  expectedWeight = 0;
//  expectedEdges = {};
//  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, false /* pathFound */, expectedWeight,
//                    expectedEdges);
//
//  auto const saturday = []() {
//    return GetUnixtimeByDate(2020, Month::Nov, Weekday::Saturday, 10 /* hh */, 00 /* mm */);
//  };
//
//  // And open from Saturday to Sunday
//  graph.SetCurrentTimeGetter(saturday);
//  expectedWeight = 3.0;
//  expectedEdges = {{0, 1}, {1, 2}, {2, 3}};
//  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
//                    expectedEdges);
//}

UNIT_TEST(RoadAccess_WayBlockedAvoidPrivateConditional)
{
  uint32_t const numVertices = 4;
  TestIndexGraphTopology graph(numVertices);

  graph.AddDirectedEdge(0, 1, 1.0);
  graph.AddDirectedEdge(0, 2, 10.0);
  graph.AddDirectedEdge(1, 3, 1.0);
  graph.AddDirectedEdge(2, 3, 10.0);

  double expectedWeight = 2.0;
  vector<TestEdge> expectedEdges = {{0, 1}, {1, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  graph.SetEdgeAccessConditional(0, 1, RoadAccess::Type::Private, "Mo-Fr 19:00 - 23:00");

  auto const mondayAlmostTwentyHalfHours = []() {
    return GetUnixtimeByDate(2020, Month::Apr, Weekday::Monday, 20 /* hh */, 30 /* mm */);
  };

  // We should avoid ways with private accesses. At 20:30 edge: 0->1 definitely has private access,
  // thus the answer is: 0->2->3.
  graph.SetCurrentTimeGetter(mondayAlmostTwentyHalfHours);
  expectedWeight = 20.0;
  expectedEdges = {{0, 2}, {2, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  graph.SetEdgeAccess(0, 2, RoadAccess::Type::No);

  // But if this is the only path (we blocked 0->2 above), we should pass through edge: 0->1 anyway.
  graph.SetCurrentTimeGetter(mondayAlmostTwentyHalfHours);
  expectedWeight = 2.0;
  expectedEdges = {{0, 1}, {1, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);
}

UNIT_TEST(RoadAccess_WayBlockedAlwaysNoExceptMonday)
{
  uint32_t const numVertices = 4;
  TestIndexGraphTopology graph(numVertices);

  graph.AddDirectedEdge(0, 1, 1.0);
  graph.AddDirectedEdge(1, 2, 1.0);
  graph.AddDirectedEdge(2, 3, 1.0);

  double expectedWeight = 3.0;
  vector<TestEdge> expectedEdges = {{0, 1}, {1, 2}, {2, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  // Always access no for edge: 1->2.
  graph.SetEdgeAccess(1, 2, RoadAccess::Type::No);
  expectedWeight = 0;
  expectedEdges = {};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, false /* pathFound */, expectedWeight,
                    expectedEdges);

  // Except Monday, access yes in this day.
  graph.SetEdgeAccessConditional(1, 2, RoadAccess::Type::Yes, "Mo");

  auto const monday = []() {
    return GetUnixtimeByDate(2020, Month::Apr, Weekday::Monday, 10 /* hh */, 00 /* mm */);
  };

  graph.SetCurrentTimeGetter(monday);
  expectedWeight = 3.0;
  expectedEdges = {{0, 1}, {1, 2}, {2, 3}};
  TestTopologyGraph(graph, 0 /* from */, 3 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);
}

UNIT_TEST(RoadAccess_WayBlockedWhenStartButOpenWhenReach)
{
  uint32_t const numVertices = 7;
  TestIndexGraphTopology graph(numVertices);

  graph.AddDirectedEdge(0, 1, 1.0);
  graph.AddDirectedEdge(1, 2, 1.0);
  graph.AddDirectedEdge(2, 3, 10800.0);
  graph.AddDirectedEdge(3, 4, 1.0);
  graph.AddDirectedEdge(4, 5, 1.0);

  // Alternative way from |3| to |5|.
  graph.AddDirectedEdge(3, 6, 1000.0);
  graph.AddDirectedEdge(6, 5, 1000.0);

  double expectedWeight = 10804.0;
  vector<TestEdge> expectedEdges = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}};
  TestTopologyGraph(graph, 0 /* from */, 5 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  auto const startAt_11_50 = []() {
    return GetUnixtimeByDate(2020, Month::Apr, Weekday::Monday, 11 /* hh */, 50 /* mm */);
  };

  graph.SetCurrentTimeGetter(startAt_11_50);
  // When we will be at |3|, current time should be:
  // 11:50:00 + (0, 1) weight + (1, 2) weight + (2, 3) weight == 14:50:01, so we should ignore
  // access: (3, 4).
  graph.SetEdgeAccessConditional(3, 4, RoadAccess::Type::No, "10:00 - 13:00");
  expectedWeight = 10804.0;
  expectedEdges = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}};
  TestTopologyGraph(graph, 0 /* from */, 5 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  auto const startAt_10_50 = []() {
    return GetUnixtimeByDate(2020, Month::Apr, Weekday::Monday, 10 /* hh */, 50 /* mm */);
  };

  graph.SetCurrentTimeGetter(startAt_10_50);
  // When we will be at |3|, current time should be:
  // 11:50:00 + (0, 1) weight + (1, 2) weight + (2, 3) weight == 12:50:01. This time places in
  // dangerous zone, but we are not sure, so we should chose alternative way.
  expectedWeight = 12802.0;
  expectedEdges = {{0, 1}, {1, 2}, {2, 3}, {3, 6}, {6, 5}};
  TestTopologyGraph(graph, 0 /* from */, 5 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  // Block alternative way.
  graph.SetEdgeAccess(3, 6, RoadAccess::Type::No);
  // We are still in dangerous zone, but alternative way is blocked, so we should chose dangerous
  // way.
  expectedWeight = 10804.0;
  expectedEdges = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}};
  TestTopologyGraph(graph, 0 /* from */, 5 /* to */, true /* pathFound */, expectedWeight,
                    expectedEdges);

  auto const startAt_9_00 = []() {
    return GetUnixtimeByDate(2020, Month::Apr, Weekday::Monday, 9 /* hh */, 00 /* mm */);
  };

  graph.SetCurrentTimeGetter(startAt_9_00);
  // If we start at 9:00:00 we will arrive at |3| at:
  // 9:00:00 + (0, 1) weight + (1, 2) weight + (2, 3) weight == 12:00:02
  // At this time are sure that (3, 4) way is blocked, so (remember that we also blocked alternative
  // way) no way should be found.
  expectedWeight = 0.0;
  expectedEdges = {};
  TestTopologyGraph(graph, 0 /* from */, 5 /* to */, false /* pathFound */, expectedWeight,
                    expectedEdges);
}
}  // namespace
