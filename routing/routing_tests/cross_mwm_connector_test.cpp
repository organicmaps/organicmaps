#include "testing/testing.hpp"

#include "routing/cross_mwm_connector_serialization.hpp"

#include "coding/writer.hpp"

using namespace routing;
using namespace std;

namespace
{
NumMwmId constexpr mwmId = 777;

void TestConnectorConsistency(CrossMwmConnector const & connector)
{
  for (Segment const & enter : connector.GetEnters())
  {
    TEST(connector.IsTransition(enter, true /* isOutgoing */), ("enter:", enter));
    TEST(!connector.IsTransition(enter, false /* isOutgoing */), ("enter:", enter));
  }

  for (Segment const & exit : connector.GetExits())
  {
    TEST(!connector.IsTransition(exit, true /* isOutgoing */), ("exit:", exit));
    TEST(connector.IsTransition(exit, false /* isOutgoing */), ("exit:", exit));
  }
}

void TestEdges(CrossMwmConnector const & connector, Segment const & from, bool isOutgoing,
               vector<SegmentEdge> const & expectedEdges)
{
  vector<SegmentEdge> edges;
  connector.GetEdgeList(from, isOutgoing, edges);
  TEST_EQUAL(edges, expectedEdges, ());
}
}

namespace routing_test
{
UNIT_TEST(OneWayEnter)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  CrossMwmConnector connector(mwmId);
  connector.AddTransition(featureId, segmentIdx, true /* oneWay */, true /* forwardIsEnter */,
                          {} /* backPoint */, {} /* frontPoint */);

  TestConnectorConsistency(connector);
  TEST_EQUAL(connector.GetEnters().size(), 1, ());
  TEST_EQUAL(connector.GetExits().size(), 0, ());
  TEST(connector.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                              true /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                               false /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                               true /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                               false /* isOutgoing */),
       ());
}

UNIT_TEST(OneWayExit)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  CrossMwmConnector connector(mwmId);
  connector.AddTransition(featureId, segmentIdx, true /* oneWay */, false /* forwardIsEnter */,
                          {} /* backPoint */, {} /* frontPoint */);

  TestConnectorConsistency(connector);
  TEST_EQUAL(connector.GetEnters().size(), 0, ());
  TEST_EQUAL(connector.GetExits().size(), 1, ());
  TEST(!connector.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                               true /* isOutgoing */),
       ());
  TEST(connector.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                              false /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                               true /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                               false /* isOutgoing */),
       ());
}

UNIT_TEST(TwoWayEnter)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  CrossMwmConnector connector(mwmId);
  connector.AddTransition(featureId, segmentIdx, false /* oneWay */, true /* forwardIsEnter */,
                          {} /* backPoint */, {} /* frontPoint */);

  TestConnectorConsistency(connector);
  TEST_EQUAL(connector.GetEnters().size(), 1, ());
  TEST_EQUAL(connector.GetExits().size(), 1, ());
  TEST(connector.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                              true /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                               false /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                               true /* isOutgoing */),
       ());
  TEST(connector.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                              false /* isOutgoing */),
       ());
}

UNIT_TEST(TwoWayExit)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  CrossMwmConnector connector(mwmId);
  connector.AddTransition(featureId, segmentIdx, false /* oneWay */, false /* forwardIsEnter */,
                          {} /* backPoint */, {} /* frontPoint */);

  TestConnectorConsistency(connector);
  TEST_EQUAL(connector.GetEnters().size(), 1, ());
  TEST_EQUAL(connector.GetExits().size(), 1, ());
  TEST(!connector.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                               true /* isOutgoing */),
       ());
  TEST(connector.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                              false /* isOutgoing */),
       ());
  TEST(connector.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                              true /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                               false /* isOutgoing */),
       ());
}

UNIT_TEST(Serialization)
{
  float constexpr kEdgesWeight = 333;

  vector<uint8_t> buffer;
  {
    vector<CrossMwmConnectorSerializer::Transition> transitions = {
        /* featureId, segmentIdx, roadMask, oneWayMask, forwardIsEnter, backPoint, frontPoint */
        {10, 1, kCarMask, kCarMask, true, m2::PointD(1.1, 1.2), m2::PointD(1.3, 1.4)},
        {20, 2, kCarMask, 0, true, m2::PointD(2.1, 2.2), m2::PointD(2.3, 2.4)},
        {30, 3, kPedestrianMask, kCarMask, true, m2::PointD(3.1, 3.2), m2::PointD(3.3, 3.4)}};

    CrossMwmConnectorPerVehicleType connectors;
    CrossMwmConnector & carConnector = connectors[static_cast<size_t>(VehicleType::Car)];
    for (auto const & transition : transitions)
      CrossMwmConnectorSerializer::AddTransition(transition, kCarMask, carConnector);

    carConnector.FillWeights(
        [](Segment const & enter, Segment const & exit) { return kEdgesWeight; });

    serial::CodingParams const codingParams;
    MemWriter<vector<uint8_t>> writer(buffer);
    CrossMwmConnectorSerializer::Serialize(transitions, connectors, codingParams, writer);
  }

  CrossMwmConnector connector(mwmId);
  {
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> source(reader);
    CrossMwmConnectorSerializer::DeserializeTransitions(VehicleType::Car, connector, source);
  }

  TestConnectorConsistency(connector);

  TEST_EQUAL(connector.GetEnters().size(), 2, ());
  TEST_EQUAL(connector.GetExits().size(), 1, ());

  TEST(!connector.IsTransition(Segment(mwmId, 0, 0, true), true /* isOutgoing */), ());

  TEST(connector.IsTransition(Segment(mwmId, 10, 1, true /* forward */), true /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, 10, 1, true /* forward */), false /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, 10, 1, false /* forward */), true /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, 10, 1, false /* forward */), false /* isOutgoing */),
       ());

  TEST(connector.IsTransition(Segment(mwmId, 20, 2, true /* forward */), true /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, 20, 2, true /* forward */), false /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, 20, 2, false /* forward */), true /* isOutgoing */),
       ());
  TEST(connector.IsTransition(Segment(mwmId, 20, 2, false /* forward */), false /* isOutgoing */),
       ());

  TEST(!connector.IsTransition(Segment(mwmId, 30, 3, true /* forward */), true /* isOutgoing */),
       ());

  TEST(!connector.WeightsWereLoaded(), ());
  TEST(!connector.HasWeights(), ());

  {
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> source(reader);
    CrossMwmConnectorSerializer::DeserializeWeights(VehicleType::Car, connector, source);
  }
  TEST(connector.WeightsWereLoaded(), ());
  TEST(connector.HasWeights(), ());

  double constexpr eps = 1e-6;
  TEST(AlmostEqualAbs(
           connector.GetPoint(Segment(mwmId, 20, 2, true /* forward */), true /* front */),
           m2::PointD(2.3, 2.4), eps),
       ());
  TEST(AlmostEqualAbs(
           connector.GetPoint(Segment(mwmId, 20, 2, true /* forward */), false /* front */),
           m2::PointD(2.1, 2.2), eps),
       ());
  TEST(AlmostEqualAbs(
           connector.GetPoint(Segment(mwmId, 20, 2, false /* forward */), true /* front */),
           m2::PointD(2.1, 2.2), eps),
       ());
  TEST(AlmostEqualAbs(
           connector.GetPoint(Segment(mwmId, 20, 2, true /* forward */), true /* front */),
           m2::PointD(2.3, 2.4), eps),
       ());

  TestEdges(connector, Segment(mwmId, 10, 1, true /* forward */), true /* isOutgoing */,
            {{Segment(mwmId, 20, 2, false /* forward */), kEdgesWeight}});

  TestEdges(connector, Segment(mwmId, 20, 2, true /* forward */), true /* isOutgoing */,
            {{Segment(mwmId, 20, 2, false /* forward */), kEdgesWeight}});

  TestEdges(connector, Segment(mwmId, 20, 2, false /* forward */), false /* isOutgoing */,
            {{Segment(mwmId, 10, 1, true /* forward */), kEdgesWeight},
             {Segment(mwmId, 20, 2, true /* forward */), kEdgesWeight}});
}
}  // namespace routing_test
