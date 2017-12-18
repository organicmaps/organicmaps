#include "testing/testing.hpp"

#include "routing/cross_mwm_connector_serialization.hpp"
#include "routing/cross_mwm_ids.hpp"

#include "coding/writer.hpp"

#include "base/osm_id.hpp"

using namespace routing;
using namespace routing::connector;
using namespace std;

namespace
{
NumMwmId constexpr mwmId = 777;

template <typename CrossMwmId>
CrossMwmConnector<CrossMwmId> CreateConnector()
{
  return CrossMwmConnector<CrossMwmId>(mwmId, 0 /* featuresNumerationOffset */);
}

template <typename CrossMwmId>
void TestConnectorConsistency(CrossMwmConnector<CrossMwmId> const & connector)
{
  for (Segment const & enter : connector.GetEnters())
  {
    TEST(connector.IsTransition(enter, false /* isOutgoing */), ("enter:", enter));
    TEST(!connector.IsTransition(enter, true /* isOutgoing */), ("enter:", enter));
  }

  for (Segment const & exit : connector.GetExits())
  {
    TEST(!connector.IsTransition(exit, false /* isOutgoing */), ("exit:", exit));
    TEST(connector.IsTransition(exit, true /* isOutgoing */), ("exit:", exit));
  }
}

template <typename CrossMwmId>
void TestOutgoingEdges(CrossMwmConnector<CrossMwmId> const & connector, Segment const & from,
                       vector<SegmentEdge> const & expectedEdges)
{
  vector<SegmentEdge> edges;
  connector.GetOutgoingEdgeList(from, edges);
  TEST_EQUAL(edges, expectedEdges, ());
}

template <typename CrossMwmId>
void TestOneWayEnter(CrossMwmId const & crossMwmId)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  auto connector = CreateConnector<CrossMwmId>();
  connector.AddTransition(crossMwmId, featureId, segmentIdx, true /* oneWay */,
                          true /* forwardIsEnter */, {} /* backPoint */, {} /* frontPoint */);

  TestConnectorConsistency(connector);
  TEST_EQUAL(connector.GetEnters().size(), 1, ());
  TEST_EQUAL(connector.GetExits().size(), 0, ());
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

template <typename CrossMwmId>
void TestOneWayExit(CrossMwmId const & crossMwmId)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  auto connector = CreateConnector<CrossMwmId>();
  connector.AddTransition(crossMwmId, featureId, segmentIdx, true /* oneWay */,
                          false /* forwardIsEnter */, {} /* backPoint */, {} /* frontPoint */);

  TestConnectorConsistency(connector);
  TEST_EQUAL(connector.GetEnters().size(), 0, ());
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
  TEST(!connector.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                               false /* isOutgoing */),
       ());
}

template <typename CrossMwmId>
void TestTwoWayEnter(CrossMwmId const & crossMwmId)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  auto connector = CreateConnector<CrossMwmId>();
  connector.AddTransition(crossMwmId, featureId, segmentIdx, false /* oneWay */,
                          true /* forwardIsEnter */, {} /* backPoint */, {} /* frontPoint */);

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
template <typename CrossMwmId>
void TestTwoWayExit(CrossMwmId const & crossMwmId)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  auto connector = CreateConnector<CrossMwmId>();
  connector.AddTransition(crossMwmId, featureId, segmentIdx, false /* oneWay */,
                          false /* forwardIsEnter */, {} /* backPoint */, {} /* frontPoint */);

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

template <typename CrossMwmId>
void TestSerialization(vector<CrossMwmConnectorSerializer::Transition<CrossMwmId>> const & transitions)
{
  double constexpr kEdgesWeight = 4444.0;

  vector<uint8_t> buffer;
  {
    CrossMwmConnectorPerVehicleType<CrossMwmId> connectors;
    CrossMwmConnector<CrossMwmId> & carConnector = connectors[static_cast<size_t>(VehicleType::Car)];
    for (auto const & transition : transitions)
      CrossMwmConnectorSerializer::AddTransition(transition, kCarMask, carConnector);

    carConnector.FillWeights(
        [&](Segment const & enter, Segment const & exit) { return kEdgesWeight; });

    serial::CodingParams const codingParams;
    MemWriter<vector<uint8_t>> writer(buffer);
    CrossMwmConnectorSerializer::Serialize(transitions, connectors, codingParams, writer);
  }

  auto connector = CreateConnector<CrossMwmId>();
  {
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> source(reader);
    CrossMwmConnectorSerializer::DeserializeTransitions(VehicleType::Car, connector, source);
  }

  TestConnectorConsistency(connector);

  TEST_EQUAL(connector.GetEnters().size(), 2, ());
  TEST_EQUAL(connector.GetExits().size(), 1, ());

  TEST(!connector.IsTransition(Segment(mwmId, 0, 0, true), true /* isOutgoing */), ());

  TEST(!connector.IsTransition(Segment(mwmId, 10, 1, true /* forward */), true /* isOutgoing */),
       ());
  TEST(connector.IsTransition(Segment(mwmId, 10, 1, true /* forward */), false /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, 10, 1, false /* forward */), true /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, 10, 1, false /* forward */), false /* isOutgoing */),
       ());

  TEST(!connector.IsTransition(Segment(mwmId, 20, 2, true /* forward */), true /* isOutgoing */),
       ());
  TEST(connector.IsTransition(Segment(mwmId, 20, 2, true /* forward */), false /* isOutgoing */),
       ());
  TEST(connector.IsTransition(Segment(mwmId, 20, 2, false /* forward */), true /* isOutgoing */),
       ());
  TEST(!connector.IsTransition(Segment(mwmId, 20, 2, false /* forward */), false /* isOutgoing */),
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

  TestOutgoingEdges(connector, Segment(mwmId, 10, 1, true /* forward */),
                    {{Segment(mwmId, 20, 2, false /* forward */),
                      RouteWeight::FromCrossMwmWeight(kEdgesWeight)}});

  TestOutgoingEdges(connector, Segment(mwmId, 20, 2, true /* forward */),
                    {{Segment(mwmId, 20, 2, false /* forward */),
                      RouteWeight::FromCrossMwmWeight(kEdgesWeight)}});
}

void GetCrossMwmId(uint32_t i, osm::Id & id) { id = osm::Id(10 * i); }

void GetCrossMwmId(uint32_t i, TransitId & id)
{
  id = TransitId(1 /* stop 1 id */, 10 * i /* stop 1 id */, 1 /* line id */);
}

template <typename CrossMwmId>
void TestWeightsSerialization()
{
  size_t constexpr kNumTransitions = 3;
  vector<double> const weights = {
      4.0, 20.0, connector::kNoRoute, 12.0, connector::kNoRoute, 40.0, 48.0, 24.0, 12.0};
  TEST_EQUAL(weights.size(), kNumTransitions * kNumTransitions, ());

  vector<uint8_t> buffer;
  {
    vector<CrossMwmConnectorSerializer::Transition<CrossMwmId>> transitions;
    for (uint32_t featureId = 0; featureId < kNumTransitions; ++featureId)
    {
      CrossMwmId id;
      GetCrossMwmId(featureId, id);
      transitions.emplace_back(id, featureId, 1 /* segmentIdx */, kCarMask, 0 /* oneWayMask */,
                               true /* forwardIsEnter */, m2::PointD::Zero(), m2::PointD::Zero());
    }

    CrossMwmConnectorPerVehicleType<CrossMwmId> connectors;
    CrossMwmConnector<CrossMwmId> & carConnector = connectors[static_cast<size_t>(VehicleType::Car)];
    for (auto const & transition : transitions)
      CrossMwmConnectorSerializer::AddTransition(transition, kCarMask, carConnector);

    int weightIdx = 0;
    carConnector.FillWeights(
        [&](Segment const & enter, Segment const & exit) { return weights[weightIdx++]; });

    serial::CodingParams const codingParams;
    MemWriter<vector<uint8_t>> writer(buffer);
    CrossMwmConnectorSerializer::Serialize(transitions, connectors, codingParams, writer);
  }

  auto connector = CreateConnector<CrossMwmId>();
  {
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> source(reader);
    CrossMwmConnectorSerializer::DeserializeTransitions(VehicleType::Car, connector, source);
  }

  TestConnectorConsistency(connector);

  TEST_EQUAL(connector.GetEnters().size(), kNumTransitions, ());
  TEST_EQUAL(connector.GetExits().size(), kNumTransitions, ());

  TEST(!connector.WeightsWereLoaded(), ());
  TEST(!connector.HasWeights(), ());

  {
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> source(reader);
    CrossMwmConnectorSerializer::DeserializeWeights(VehicleType::Car, connector, source);
  }
  TEST(connector.WeightsWereLoaded(), ());
  TEST(connector.HasWeights(), ());

  int weightIdx = 0;

  for (uint32_t enterId = 0; enterId < kNumTransitions; ++enterId)
  {
    Segment const enter(mwmId, enterId, 1, true /* forward */);
    vector<SegmentEdge> expectedEdges;
    for (uint32_t exitId = 0; exitId < kNumTransitions; ++exitId)
    {
      auto const weight = weights[weightIdx];
      if (weight != connector::kNoRoute)
      {
        expectedEdges.emplace_back(Segment(mwmId, exitId, 1 /* segmentIdx */, false /* forward */),
                                   RouteWeight::FromCrossMwmWeight(weight));
      }
      ++weightIdx;
    }

    TestOutgoingEdges(connector, enter, expectedEdges);
  }
}
}  // namespace

namespace routing_test
{
UNIT_TEST(OneWayEnter)
{
  TestOneWayEnter(osm::Id(1ULL));
  TestOneWayEnter(TransitId(1 /* stop 1 id */, 2 /* stop 2 id */, 1 /* line id */));
}

UNIT_TEST(OneWayExit)
{
  TestOneWayExit(osm::Id(1ULL));
  TestOneWayExit(TransitId(1 /* stop 1 id */, 2 /* stop 2 id */, 1 /* line id */));
}

UNIT_TEST(TwoWayEnter)
{
  TestTwoWayEnter(osm::Id(1ULL));
  TestTwoWayEnter(TransitId(1 /* stop 1 id */, 2 /* stop 2 id */, 1 /* line id */));
}

UNIT_TEST(TwoWayExit)
{
  TestTwoWayExit(osm::Id(1ULL));
  TestTwoWayExit(TransitId(1 /* stop 1 id */, 2 /* stop 2 id */, 1 /* line id */));
}

UNIT_TEST(Serialization)
{
  {
    vector<CrossMwmConnectorSerializer::Transition<osm::Id>> const transitions = {
        /* osmId featureId, segmentIdx, roadMask, oneWayMask, forwardIsEnter, backPoint, frontPoint */
        {osm::Id(100ULL), 10, 1, kCarMask, kCarMask, true, m2::PointD(1.1, 1.2),
         m2::PointD(1.3, 1.4)},
        {osm::Id(200ULL), 20, 2, kCarMask, 0, true, m2::PointD(2.1, 2.2), m2::PointD(2.3, 2.4)},
        {osm::Id(300ULL), 30, 3, kPedestrianMask, kCarMask, true, m2::PointD(3.1, 3.2),
         m2::PointD(3.3, 3.4)}};
    TestSerialization(transitions);
  }
  {
    vector<CrossMwmConnectorSerializer::Transition<TransitId>> const transitions = {
        /* osmId featureId, segmentIdx, roadMask, oneWayMask, forwardIsEnter, backPoint, frontPoint */
        {TransitId(1ULL /* stop 1 id */, 2ULL /* stop 2 id */, 1ULL /* line id */), 10, 1, kCarMask,
         kCarMask, true, m2::PointD(1.1, 1.2), m2::PointD(1.3, 1.4)},
        {TransitId(1ULL, 3ULL, 1ULL), 20, 2, kCarMask, 0, true, m2::PointD(2.1, 2.2),
         m2::PointD(2.3, 2.4)},
        {TransitId(1ULL, 3ULL, 2ULL), 30, 3, kPedestrianMask, kCarMask, true, m2::PointD(3.1, 3.2),
         m2::PointD(3.3, 3.4)}};
    TestSerialization(transitions);
  }
}

UNIT_TEST(WeightsSerialization)
{
  TestWeightsSerialization<osm::Id>();
  TestWeightsSerialization<TransitId>();
}
}  // namespace routing_test
