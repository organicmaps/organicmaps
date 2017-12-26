#include "testing/testing.hpp"

#include "routing/road_access.hpp"
#include "routing/road_access_serialization.hpp"
#include "routing/routing_tests/index_graph_tools.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

using namespace routing;
using namespace routing_test;
using namespace std;

using TestEdge = TestIndexGraphTopology::Edge;

namespace
{
UNIT_TEST(RoadAccess_Serialization)
{
  // Segment is (numMwmId, featureId, segmentIdx, isForward).
  map<uint32_t, RoadAccess::Type> const m0 = {
      {1, RoadAccess::Type::No},
      {2, RoadAccess::Type::Private},
  };

  map<uint32_t, RoadAccess::Type> const m1 = {
      {1, RoadAccess::Type::Private},
      {2, RoadAccess::Type::Destination},
  };

  RoadAccess roadAccessCar;
  roadAccessCar.SetFeatureTypesForTests(m0);

  RoadAccess roadAccessPedestrian;
  roadAccessPedestrian.SetFeatureTypesForTests(m1);

  RoadAccessSerializer::RoadAccessByVehicleType roadAccessAllTypes;
  roadAccessAllTypes[static_cast<size_t>(VehicleType::Car)] = roadAccessCar;
  roadAccessAllTypes[static_cast<size_t>(VehicleType::Pedestrian)] = roadAccessPedestrian;

  vector<uint8_t> buf;
  {
    MemWriter<decltype(buf)> writer(buf);
    RoadAccessSerializer::Serialize(writer, roadAccessAllTypes);
  }

  {
    RoadAccess deserializedRoadAccess;

    MemReader memReader(buf.data(), buf.size());
    ReaderSource<MemReader> src(memReader);
    RoadAccessSerializer::Deserialize(src, VehicleType::Car, deserializedRoadAccess);
    TEST_EQUAL(src.Size(), 0, ());

    TEST_EQUAL(roadAccessCar, deserializedRoadAccess, ());
  }

  {
    RoadAccess deserializedRoadAccess;

    MemReader memReader(buf.data(), buf.size());
    ReaderSource<MemReader> src(memReader);
    RoadAccessSerializer::Deserialize(src, VehicleType::Pedestrian, deserializedRoadAccess);
    TEST_EQUAL(src.Size(), 0, ());

    TEST_EQUAL(roadAccessPedestrian, deserializedRoadAccess, ());
  }
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
