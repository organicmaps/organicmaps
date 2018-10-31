#include "testing/testing.hpp"

#include "routing/fake_graph.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <set>

using namespace routing;
using namespace std;

namespace
{
// Constructs simple fake graph where vertex (i + 1) is child of vertex (i) with |numFake| fake
// vertices and |numReal| real vertices. Checks vetex-to-segment, segment-to-vertex, fake-to-real,
// real-to-fake mappings for each vertex. Checks ingoing and outgoing sets.
FakeGraph<int32_t /* SegmentType */, m2::PointD /* VertexType */>
ConstructFakeGraph(uint32_t numerationStart, uint32_t numFake, uint32_t numReal)
{
  FakeGraph<int32_t, m2::PointD> fakeGraph;

  TEST_EQUAL(fakeGraph.GetSize(), 0, ("Constructed fake graph not empty"));
  if (numFake < 1)
  {
    CHECK_EQUAL(numReal, 0,
                ("Construction of non-empty fake graph without pure fake vertices not supported."));
    return fakeGraph;
  }

  int32_t const startSegment = numerationStart;
  auto const startVertex = m2::PointD(numerationStart, numerationStart);
  fakeGraph.AddStandaloneVertex(startSegment, startVertex);

  // Add pure fake.
  for (uint32_t prevNumber = numerationStart; prevNumber + 1 < numerationStart + numFake + numReal;
       ++prevNumber)
  {
    bool const newIsReal = prevNumber + 1 < numerationStart + numFake ? false : true;
    int32_t const prevSegment = prevNumber;
    int32_t const newSegment = prevNumber + 1;
    auto const newVertex = m2::PointD(prevNumber + 1, prevNumber + 1);
    int32_t const realSegment = -(prevNumber + 1);

    fakeGraph.AddVertex(prevSegment, newSegment, newVertex, true /* isOutgoing */,
                        newIsReal /* isPartOfReal */, realSegment);

    // Test segment to vertex mapping.
    TEST_EQUAL(fakeGraph.GetVertex(newSegment), newVertex, ("Wrong segment to vertex mapping."));
    // Test outgoing edge.
    TEST_EQUAL(fakeGraph.GetEdges(newSegment, false /* isOutgoing */), set<int32_t>{prevSegment},
               ("Wrong ingoing edges set."));
    // Test ingoing edge.
    TEST_EQUAL(fakeGraph.GetEdges(prevSegment, true /* isOutgoing */), set<int32_t>{newSegment},
               ("Wrong ingoing edges set."));
    // Test graph size
    TEST_EQUAL(fakeGraph.GetSize() + numerationStart, prevNumber + 2, ("Wrong fake graph size."));
    // Test fake to real and real to fake mapping.
    int32_t realFound;
    if (newIsReal)
    {
      TEST_EQUAL(fakeGraph.FindReal(newSegment, realFound), true,
                 ("Unexpected real segment found."));
      TEST_EQUAL(realSegment, realFound, ("Wrong fake to real mapping."));
      TEST_EQUAL(fakeGraph.GetFake(realSegment), set<int32_t>{newSegment},
                 ("Unexpected fake segment found."));
    }
    else
    {
      TEST_EQUAL(fakeGraph.FindReal(newSegment, realFound), false,
                 ("Unexpected real segment found."));
      TEST(fakeGraph.GetFake(realSegment).empty(), ("Unexpected fake segment found."));
    }
  }
  return fakeGraph;
}
}  // namespace

namespace routing
{
// Test constructs two fake graphs, performs checks during construction, merges graphs
// using FakeGraph::Append and checks topology of merged graph.
UNIT_TEST(FakeGraphTest)
{
  uint32_t const fake0 = 5;
  uint32_t const real0 = 3;
  uint32_t const fake1 = 4;
  uint32_t const real1 = 2;
  auto fakeGraph0 = ConstructFakeGraph(0, fake0, real0);
  TEST_EQUAL(fakeGraph0.GetSize(), fake0 + real0, ("Wrong fake graph size"));

  auto const fakeGraph1 =
      ConstructFakeGraph(static_cast<uint32_t>(fakeGraph0.GetSize()), fake1, real1);
  TEST_EQUAL(fakeGraph1.GetSize(), fake1 + real1, ("Wrong fake graph size"));

  fakeGraph0.Append(fakeGraph1);
  TEST_EQUAL(fakeGraph0.GetSize(), fake0 + real0 + fake1 + real1, ("Wrong size of merged graph"));

  // Test merged graph.
  for (uint32_t i = 0; i + 1 < fakeGraph0.GetSize(); ++i)
  {
    int32_t const segmentFrom = i;
    int32_t const segmentTo = i + 1;

    TEST_EQUAL(fakeGraph0.GetVertex(segmentFrom), m2::PointD(i, i), ("Wrong segment to vertex mapping."));
    TEST_EQUAL(fakeGraph0.GetVertex(segmentTo), m2::PointD(i + 1, i + 1), ("Wrong segment to vertex mapping."));
    // No connection to next fake segment; next segment was in separate fake graph before Append.
    if (i + 1 == fake0 + real0)
    {
      TEST(fakeGraph0.GetEdges(segmentFrom, true /* isOutgoing */).empty(),
           ("Wrong ingoing edges set."));
      TEST(fakeGraph0.GetEdges(segmentTo, false /* isOutgoing */).empty(),
           ("Wrong ingoing edges set."));
    }
    else
    {
      TEST_EQUAL(fakeGraph0.GetEdges(segmentFrom, true /* isOutgoing */), set<int32_t>{segmentTo},
                 ("Wrong ingoing edges set."));
      TEST_EQUAL(fakeGraph0.GetEdges(segmentTo, false /* isOutgoing */),
                 set<int32_t>{segmentFrom}, ("Wrong ingoing edges set."));
    }
  }
}
}  // namespace routing
