#include "testing/testing.hpp"

#include "routing/road_access.hpp"
#include "routing/road_access_serialization.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <cstdint>
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

void FillRoadAccessBySampleConditional_1(RoadAccess & roadAccess)
{
  std::vector<std::string> const openingHoursStrings = {
      "Mo-Su", "10:00-18:00", "Mo-Fr 10:00-14:00", "09:00-13:00", "Apr - May", "2010 - 2100"};

  std::vector<osmoh::OpeningHours> openingHours;
  for (auto const & oh : openingHoursStrings)
  {
    openingHours.emplace_back(oh);
    TEST(openingHours.back().IsValid(), ());
  }

  RoadAccess::Conditional conditional_1;
  conditional_1.Insert(RoadAccess::Type::No, move(openingHours[0]));
  conditional_1.Insert(RoadAccess::Type::Private, move(openingHours[1]));

  RoadAccess::Conditional conditional_2;
  conditional_2.Insert(RoadAccess::Type::Destination, move(openingHours[2]));

  RoadAccess::Conditional conditional_3;
  conditional_3.Insert(RoadAccess::Type::No, move(openingHours[4]));
  conditional_3.Insert(RoadAccess::Type::Destination, move(openingHours[3]));

  RoadAccess::Conditional conditional_4;
  conditional_4.Insert(RoadAccess::Type::Destination, move(openingHours[5]));

  RoadAccess::WayToAccessConditional wayToAccessConditional = {{1 /* featureId */, conditional_1},
                                                               {2 /* featureId */, conditional_2}};

  RoadAccess::PointToAccessConditional pointToAccessConditional = {
      {RoadPoint(3 /* featureId */, 0 /* pointId */), conditional_3},
      {RoadPoint(4 /* featureId */, 7 /* pointId */), conditional_4}};

  roadAccess.SetAccessConditional(move(wayToAccessConditional), move(pointToAccessConditional));
}

void FillRoadAccessBySampleConditional_2(RoadAccess & roadAccess)
{
  std::vector<std::string> const openingHoursStrings = {
      "Mo", "Apr-May 03:00-04:25", "Mo-Sa 12:00-13:00", "2010-2098", "Nov-Apr", "19:00-21:00"};

  std::vector<osmoh::OpeningHours> openingHours;
  for (auto const & oh : openingHoursStrings)
  {
    openingHours.emplace_back(oh);
    TEST(openingHours.back().IsValid(), (oh));
  }

  RoadAccess::Conditional conditional_1;
  conditional_1.Insert(RoadAccess::Type::Private, move(openingHours[0]));

  RoadAccess::Conditional conditional_2;
  conditional_2.Insert(RoadAccess::Type::No, move(openingHours[1]));
  conditional_2.Insert(RoadAccess::Type::Private, move(openingHours[2]));

  RoadAccess::Conditional conditional_3;
  conditional_3.Insert(RoadAccess::Type::Destination, move(openingHours[3]));

  RoadAccess::Conditional conditional_4;
  conditional_4.Insert(RoadAccess::Type::No, move(openingHours[4]));
  conditional_4.Insert(RoadAccess::Type::No, move(openingHours[5]));

  RoadAccess::WayToAccessConditional wayToAccessConditional = {{1 /* featureId */, conditional_1},
                                                               {2 /* featureId */, conditional_2}};

  RoadAccess::PointToAccessConditional pointToAccessConditional = {
      {RoadPoint(3 /* featureId */, 10 /* pointId */), conditional_3},
      {RoadPoint(4 /* featureId */, 2 /* pointId */), conditional_4}};

  roadAccess.SetAccessConditional(move(wayToAccessConditional), move(pointToAccessConditional));
}


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
    RoadAccessSerializer::Deserialize(src, vehicleType, deserializedRoadAccess);
    TEST_EQUAL(src.Size(), 0, ());
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

UNIT_CLASS_TEST(RoadAccessSerDesTest, RoadAccess_Serdes_Conditional_One_Vehicle)
{
  auto constexpr kVehicleTypeCount = static_cast<size_t>(VehicleType::Count);
  for (size_t vehicleTypeId = 0; vehicleTypeId < kVehicleTypeCount; ++vehicleTypeId)
  {
    RoadAccess roadAccess;
    FillRoadAccessBySampleConditional_1(roadAccess);

    RoadAccessSerializer::RoadAccessByVehicleType roadAccessAllTypes;
    roadAccessAllTypes[vehicleTypeId] = roadAccess;

    Serialize(roadAccessAllTypes);
    TestDeserialize(static_cast<VehicleType>(vehicleTypeId), roadAccess);
    ClearBuffer();
  }
}

UNIT_CLASS_TEST(RoadAccessSerDesTest, RoadAccess_Serdes_Conditional_Several_Vehicles)
{
  RoadAccess roadAccessCar;
  FillRoadAccessBySampleConditional_1(roadAccessCar);

  RoadAccess roadAccessPedestrian;
  FillRoadAccessBySampleConditional_2(roadAccessPedestrian);

  RoadAccessSerializer::RoadAccessByVehicleType roadAccessAllTypes;
  roadAccessAllTypes[static_cast<size_t>(VehicleType::Car)] = roadAccessCar;
  roadAccessAllTypes[static_cast<size_t>(VehicleType::Pedestrian)] = roadAccessPedestrian;

  Serialize(roadAccessAllTypes);
  TestDeserialize(VehicleType::Car, roadAccessCar);
  TestDeserialize(VehicleType::Pedestrian, roadAccessPedestrian);
}

UNIT_CLASS_TEST(RoadAccessSerDesTest, RoadAccess_Serdes_Conditional_Mixed_Several_Vehicles)
{
  RoadAccess roadAccessCar;
  FillRoadAccessBySampleConditional_1(roadAccessCar);
  FillRoadAccessBySample_1(roadAccessCar);

  RoadAccess roadAccessPedestrian;
  FillRoadAccessBySampleConditional_2(roadAccessPedestrian);
  FillRoadAccessBySample_2(roadAccessPedestrian);

  RoadAccessSerializer::RoadAccessByVehicleType roadAccessAllTypes;
  roadAccessAllTypes[static_cast<size_t>(VehicleType::Car)] = roadAccessCar;
  roadAccessAllTypes[static_cast<size_t>(VehicleType::Pedestrian)] = roadAccessPedestrian;

  Serialize(roadAccessAllTypes);
  TestDeserialize(VehicleType::Car, roadAccessCar);
  TestDeserialize(VehicleType::Pedestrian, roadAccessPedestrian);
}

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
}  // namespace
