#include "routing/fake_graph.hpp"

#include "testing/testing.hpp"

#include "routing/fake_feature_ids.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/segment.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <cstdint>
#include <set>

namespace fake_graph_test
{
using namespace routing;
using namespace std;

Segment GetSegment(uint32_t segmentIdx, bool isReal = false)
{
  static NumMwmId constexpr kFakeNumMwmId = std::numeric_limits<NumMwmId>::max();
  static uint32_t constexpr kFakeFeatureId = FakeFeatureIds::kIndexGraphStarterId;
  if (isReal)
    return Segment(0 /* mwmId */, 0 /* featureId */, segmentIdx, true /* isForward */);
  return Segment(kFakeNumMwmId, kFakeFeatureId, segmentIdx, true /* isForward */);
}

FakeVertex GetFakeVertex(uint32_t index)
{
  auto const indexCoord = static_cast<double>(index);
  LatLonWithAltitude const coord({indexCoord, indexCoord}, 0 /* altitude */);
  //  FakeVertex(NumMwmId numMwmId, LatLonWithAltitude const & from,
  //             LatLonWithAltitude const & to, Type type)
  return FakeVertex(0 /* mwmId */, coord, coord, FakeVertex::Type::PureFake);
}

// Constructs simple fake graph where vertex (i + 1) is child of vertex (i) with |numFake| fake
// vertices and |numReal| real vertices. Checks vertex-to-segment, segment-to-vertex, fake-to-real,
// real-to-fake mappings for each vertex. Checks ingoing and outgoing sets.
FakeGraph ConstructFakeGraph(uint32_t numerationStart, uint32_t numFake, uint32_t numReal)
{
  FakeGraph fakeGraph;
  TEST_EQUAL(fakeGraph.GetSize(), 0, ("Constructed fake graph not empty"));
  if (numFake < 1)
  {
    CHECK_EQUAL(numReal, 0, ("Construction of non-empty fake graph without pure fake vertices not supported."));
    return fakeGraph;
  }

  auto const startSegment = GetSegment(numerationStart);
  auto const startVertex = GetFakeVertex(numerationStart);
  fakeGraph.AddStandaloneVertex(startSegment, startVertex);

  // Add pure fake.
  for (uint32_t prevNumber = numerationStart; prevNumber + 1 < numerationStart + numFake + numReal; ++prevNumber)
  {
    bool const newIsReal = prevNumber + 1 >= numerationStart + numFake;
    auto const prevSegment = GetSegment(prevNumber);
    auto const newSegment = GetSegment(prevNumber + 1);
    auto const newVertex = GetFakeVertex(prevNumber + 1);
    auto const realSegment = GetSegment(prevNumber + 1, true /* isReal */);

    fakeGraph.AddVertex(prevSegment, newSegment, newVertex, true /* isOutgoing */, newIsReal /* isPartOfReal */,
                        realSegment);

    // Test segment to vertex mapping.
    TEST_EQUAL(fakeGraph.GetVertex(newSegment), newVertex, ("Wrong segment to vertex mapping."));
    // Test outgoing edge.
    TEST_EQUAL(fakeGraph.GetEdges(newSegment, false /* isOutgoing */), set<Segment>{prevSegment},
               ("Wrong ingoing edges set."));
    // Test ingoing edge.
    TEST_EQUAL(fakeGraph.GetEdges(prevSegment, true /* isOutgoing */), set<Segment>{newSegment},
               ("Wrong ingoing edges set."));
    // Test graph size
    TEST_EQUAL(fakeGraph.GetSize() + numerationStart, prevNumber + 2, ("Wrong fake graph size."));
    // Test fake to real and real to fake mapping.
    Segment realFound;
    if (newIsReal)
    {
      TEST_EQUAL(fakeGraph.FindReal(newSegment, realFound), true, ("Unexpected real segment found."));
      TEST_EQUAL(realSegment, realFound, ("Wrong fake to real mapping."));
      TEST_EQUAL(fakeGraph.GetFake(realSegment), set<Segment>{newSegment}, ("Unexpected fake segment found."));
    }
    else
    {
      TEST_EQUAL(fakeGraph.FindReal(newSegment, realFound), false, ("Unexpected real segment found."));
      TEST(fakeGraph.GetFake(realSegment).empty(), ("Unexpected fake segment found."));
    }
  }
  return fakeGraph;
}

// Test constructs two fake graphs, performs checks during construction, merges graphs
// Calls FakeGraph::Append and checks topology of the merged graph.
UNIT_TEST(FakeGraphTest)
{
  uint32_t const fake0 = 5;
  uint32_t const real0 = 3;
  uint32_t const fake1 = 4;
  uint32_t const real1 = 2;
  auto fakeGraph0 = ConstructFakeGraph(0, fake0, real0);
  TEST_EQUAL(fakeGraph0.GetSize(), fake0 + real0, ("Wrong fake graph size"));

  auto const fakeGraph1 = ConstructFakeGraph(static_cast<uint32_t>(fakeGraph0.GetSize()), fake1, real1);
  TEST_EQUAL(fakeGraph1.GetSize(), fake1 + real1, ("Wrong fake graph size"));

  fakeGraph0.Append(fakeGraph1);
  TEST_EQUAL(fakeGraph0.GetSize(), fake0 + real0 + fake1 + real1, ("Wrong size of merged graph"));

  // Test merged graph.
  for (uint32_t i = 0; i + 1 < fakeGraph0.GetSize(); ++i)
  {
    auto const segmentFrom = GetSegment(i);
    auto const segmentTo = GetSegment(i + 1);

    TEST_EQUAL(fakeGraph0.GetVertex(segmentFrom), GetFakeVertex(i), ("Wrong segment to vertex mapping."));
    TEST_EQUAL(fakeGraph0.GetVertex(segmentTo), GetFakeVertex(i + 1), ("Wrong segment to vertex mapping."));
    // No connection to next fake segment; next segment was in separate fake graph before Append.
    if (i + 1 == fake0 + real0)
    {
      TEST(fakeGraph0.GetEdges(segmentFrom, true /* isOutgoing */).empty(), ("Wrong ingoing edges set."));
      TEST(fakeGraph0.GetEdges(segmentTo, false /* isOutgoing */).empty(), ("Wrong ingoing edges set."));
    }
    else
    {
      TEST_EQUAL(fakeGraph0.GetEdges(segmentFrom, true /* isOutgoing */), set<Segment>{segmentTo},
                 ("Wrong ingoing edges set."));
      TEST_EQUAL(fakeGraph0.GetEdges(segmentTo, false /* isOutgoing */), set<Segment>{segmentFrom},
                 ("Wrong ingoing edges set."));
    }
  }
}
}  // namespace fake_graph_test
