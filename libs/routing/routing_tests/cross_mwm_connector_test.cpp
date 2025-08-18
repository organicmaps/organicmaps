#include "testing/testing.hpp"

#include "routing/cross_mwm_connector_serialization.hpp"
#include "routing/cross_mwm_ids.hpp"

#include "coding/writer.hpp"

#include "base/geo_object_id.hpp"

namespace cross_mwm_connector_test
{
using namespace routing;
using namespace routing::connector;
using namespace std;

namespace
{
NumMwmId constexpr kTestMwmId = 777;

template <typename CrossMwmId>
struct CrossMwmBuilderTestFixture
{
  CrossMwmConnector<CrossMwmId> connector;
  CrossMwmConnectorBuilder<CrossMwmId> builder;

  explicit CrossMwmBuilderTestFixture(NumMwmId numMwmId = kTestMwmId) : connector(numMwmId), builder(connector) {}
};

template <typename CrossMwmId>
void TestConnectorConsistency(CrossMwmConnector<CrossMwmId> const & connector)
{
  connector.ForEachEnter([&connector](uint32_t, Segment const & enter)
  {
    TEST(connector.IsTransition(enter, false /* isOutgoing */), ("enter:", enter));
    TEST(!connector.IsTransition(enter, true /* isOutgoing */), ("enter:", enter));
  });

  connector.ForEachExit([&connector](uint32_t, Segment const & exit)
  {
    TEST(!connector.IsTransition(exit, false /* isOutgoing */), ("exit:", exit));
    TEST(connector.IsTransition(exit, true /* isOutgoing */), ("exit:", exit));
  });
}

template <typename CrossMwmId>
void TestOutgoingEdges(CrossMwmConnector<CrossMwmId> const & connector, Segment const & from,
                       vector<SegmentEdge> const & expectedEdges)
{
  typename CrossMwmConnector<CrossMwmId>::EdgeListT edges;
  connector.GetOutgoingEdgeList(from, edges);

  TEST_EQUAL(edges.size(), expectedEdges.size(), ());
  for (size_t i = 0; i < edges.size(); ++i)
    TEST_EQUAL(edges[i], expectedEdges[i], ());
}

template <typename CrossMwmId>
void TestOneWayEnter(CrossMwmId const & crossMwmId)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;

  CrossMwmBuilderTestFixture<CrossMwmId> test;
  test.builder.AddTransition(crossMwmId, featureId, segmentIdx, true /* oneWay */, true /* forwardIsEnter */);

  TestConnectorConsistency(test.connector);
  TEST_EQUAL(test.connector.GetNumEnters(), 1, ());
  TEST_EQUAL(test.connector.GetNumExits(), 0, ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, true /* forward */),
                                    true /* isOutgoing */),
       ());
  TEST(test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, true /* forward */),
                                   false /* isOutgoing */),
       ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, false /* forward */),
                                    true /* isOutgoing */),
       ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, false /* forward */),
                                    false /* isOutgoing */),
       ());
}

template <typename CrossMwmId>
void TestOneWayExit(CrossMwmId const & crossMwmId)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;

  CrossMwmBuilderTestFixture<CrossMwmId> test;
  test.builder.AddTransition(crossMwmId, featureId, segmentIdx, true /* oneWay */, false /* forwardIsEnter */);

  TestConnectorConsistency(test.connector);
  TEST_EQUAL(test.connector.GetNumEnters(), 0, ());
  TEST_EQUAL(test.connector.GetNumExits(), 1, ());
  TEST(test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, true /* forward */),
                                   true /* isOutgoing */),
       ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, true /* forward */),
                                    false /* isOutgoing */),
       ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, false /* forward */),
                                    true /* isOutgoing */),
       ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, false /* forward */),
                                    false /* isOutgoing */),
       ());
}

template <typename CrossMwmId>
void TestTwoWayEnter(CrossMwmId const & crossMwmId)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;

  CrossMwmBuilderTestFixture<CrossMwmId> test;
  test.builder.AddTransition(crossMwmId, featureId, segmentIdx, false /* oneWay */, true /* forwardIsEnter */);

  TestConnectorConsistency(test.connector);
  TEST_EQUAL(test.connector.GetNumEnters(), 1, ());
  TEST_EQUAL(test.connector.GetNumExits(), 1, ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, true /* forward */),
                                    true /* isOutgoing */),
       ());
  TEST(test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, true /* forward */),
                                   false /* isOutgoing */),
       ());
  TEST(test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, false /* forward */),
                                   true /* isOutgoing */),
       ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, false /* forward */),
                                    false /* isOutgoing */),
       ());
}

template <typename CrossMwmId>
void TestTwoWayExit(CrossMwmId const & crossMwmId)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;

  CrossMwmBuilderTestFixture<CrossMwmId> test;
  test.builder.AddTransition(crossMwmId, featureId, segmentIdx, false /* oneWay */, false /* forwardIsEnter */);

  TestConnectorConsistency(test.connector);
  TEST_EQUAL(test.connector.GetNumEnters(), 1, ());
  TEST_EQUAL(test.connector.GetNumExits(), 1, ());
  TEST(test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, true /* forward */),
                                   true /* isOutgoing */),
       ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, true /* forward */),
                                    false /* isOutgoing */),
       ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, false /* forward */),
                                    true /* isOutgoing */),
       ());
  TEST(test.connector.IsTransition(Segment(kTestMwmId, featureId, segmentIdx, false /* forward */),
                                   false /* isOutgoing */),
       ());
}

template <typename CrossMwmId>
void TestSerialization(CrossMwmConnectorBuilderEx<CrossMwmId> & builder)
{
  double constexpr kEdgesWeight = 4444.0;

  vector<uint8_t> buffer;
  builder.PrepareConnector(VehicleType::Car);
  builder.FillWeights([&](Segment const &, Segment const &) { return kEdgesWeight; });

  MemWriter<vector<uint8_t>> writer(buffer);
  builder.Serialize(writer);

  MemReader reader(buffer.data(), buffer.size());

  CrossMwmBuilderTestFixture<CrossMwmId> test;

  test.builder.DeserializeTransitions(VehicleType::Car, reader);

  TestConnectorConsistency(test.connector);

  TEST_EQUAL(test.connector.GetNumEnters(), 2, ());
  TEST_EQUAL(test.connector.GetNumExits(), 1, ());

  TEST(!test.connector.IsTransition(Segment(kTestMwmId, 0, 0, true), true /* isOutgoing */), ());

  TEST(!test.connector.IsTransition(Segment(kTestMwmId, 10, 1, true /* forward */), true /* isOutgoing */), ());
  TEST(test.connector.IsTransition(Segment(kTestMwmId, 10, 1, true /* forward */), false /* isOutgoing */), ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, 10, 1, false /* forward */), true /* isOutgoing */), ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, 10, 1, false /* forward */), false /* isOutgoing */), ());

  TEST(!test.connector.IsTransition(Segment(kTestMwmId, 20, 2, true /* forward */), true /* isOutgoing */), ());
  TEST(test.connector.IsTransition(Segment(kTestMwmId, 20, 2, true /* forward */), false /* isOutgoing */), ());
  TEST(test.connector.IsTransition(Segment(kTestMwmId, 20, 2, false /* forward */), true /* isOutgoing */), ());
  TEST(!test.connector.IsTransition(Segment(kTestMwmId, 20, 2, false /* forward */), false /* isOutgoing */), ());

  TEST(!test.connector.IsTransition(Segment(kTestMwmId, 30, 3, true /* forward */), true /* isOutgoing */), ());

  TEST(!test.connector.WeightsWereLoaded(), ());
  TEST(!test.connector.HasWeights(), ());

  test.builder.DeserializeWeights(reader);

  TEST(test.connector.WeightsWereLoaded(), ());
  TEST(test.connector.HasWeights(), ());

  TestOutgoingEdges(test.connector, Segment(kTestMwmId, 10, 1, true /* forward */),
                    {{Segment(kTestMwmId, 20, 2, false /* forward */), RouteWeight::FromCrossMwmWeight(kEdgesWeight)}});

  TestOutgoingEdges(test.connector, Segment(kTestMwmId, 20, 2, true /* forward */),
                    {{Segment(kTestMwmId, 20, 2, false /* forward */), RouteWeight::FromCrossMwmWeight(kEdgesWeight)}});
}

void GetCrossMwmId(uint32_t i, base::GeoObjectId & id)
{
  id = base::GeoObjectId(base::MakeOsmWay(10 * i));
}

void GetCrossMwmId(uint32_t i, TransitId & id)
{
  id = TransitId(1 /* stop 1 id */, 10 * i /* stop 1 id */, 1 /* line id */);
}

template <typename CrossMwmId>
void TestWeightsSerialization()
{
  size_t constexpr kNumTransitions = 3;
  uint32_t constexpr segmentIdx = 1;
  double const weights[] = {4.0, 20.0, connector::kNoRoute, 12.0, connector::kNoRoute, 40.0, 48.0, 24.0, 12.0};
  TEST_EQUAL(std::size(weights), kNumTransitions * kNumTransitions, ());

  std::map<std::pair<Segment, Segment>, double> expectedWeights;

  vector<uint8_t> buffer;
  {
    CrossMwmConnectorBuilderEx<CrossMwmId> builder;
    for (uint32_t featureId : {2, 0, 1})  // reshuffled
    {
      CrossMwmId id;
      GetCrossMwmId(featureId, id);
      builder.AddTransition(id, featureId, segmentIdx, kCarMask, 0 /* oneWayMask */, true /* forwardIsEnter */);
    }

    builder.PrepareConnector(VehicleType::Car);

    size_t weightIdx = 0;
    builder.FillWeights([&](Segment const & enter, Segment const & exit)
    {
      double const w = weights[weightIdx++];
      TEST(expectedWeights.insert({{enter, exit}, w}).second, ());
      return w;
    });

    MemWriter<vector<uint8_t>> writer(buffer);
    builder.Serialize(writer);
  }

  MemReader reader(buffer.data(), buffer.size());

  NumMwmId const mwmId = kGeneratorMwmId;
  CrossMwmBuilderTestFixture<CrossMwmId> test(mwmId);

  test.builder.DeserializeTransitions(VehicleType::Car, reader);

  TestConnectorConsistency(test.connector);

  TEST_EQUAL(test.connector.GetNumEnters(), kNumTransitions, ());
  TEST_EQUAL(test.connector.GetNumExits(), kNumTransitions, ());

  TEST(!test.connector.WeightsWereLoaded(), ());
  TEST(!test.connector.HasWeights(), ());

  test.builder.DeserializeWeights(reader);

  TEST(test.connector.WeightsWereLoaded(), ());
  TEST(test.connector.HasWeights(), ());

  for (uint32_t enterId = 0; enterId < kNumTransitions; ++enterId)
  {
    vector<SegmentEdge> expectedEdges;

    Segment const enter(mwmId, enterId, segmentIdx, true /* forward */);
    for (uint32_t exitId = 0; exitId < kNumTransitions; ++exitId)
    {
      Segment const exit(mwmId, exitId, segmentIdx, false /* forward */);

      auto const it = expectedWeights.find({enter, exit});
      TEST(it != expectedWeights.end(), ());
      if (it->second != connector::kNoRoute)
        expectedEdges.emplace_back(exit, RouteWeight::FromCrossMwmWeight(it->second));
    }

    TestOutgoingEdges(test.connector, enter, expectedEdges);
  }
}
}  // namespace

UNIT_TEST(CMWMC_OneWayEnter)
{
  TestOneWayEnter(base::MakeOsmWay(1ULL));
  TestOneWayEnter(TransitId(1 /* stop 1 id */, 2 /* stop 2 id */, 1 /* line id */));
}

UNIT_TEST(CMWMC_OneWayExit)
{
  TestOneWayExit(base::MakeOsmWay(1ULL));
  TestOneWayExit(TransitId(1 /* stop 1 id */, 2 /* stop 2 id */, 1 /* line id */));
}

UNIT_TEST(CMWMC_TwoWayEnter)
{
  TestTwoWayEnter(base::MakeOsmWay(1ULL));
  TestTwoWayEnter(TransitId(1 /* stop 1 id */, 2 /* stop 2 id */, 1 /* line id */));
}

UNIT_TEST(CMWMC_TwoWayExit)
{
  TestTwoWayExit(base::MakeOsmWay(1ULL));
  TestTwoWayExit(TransitId(1 /* stop 1 id */, 2 /* stop 2 id */, 1 /* line id */));
}

UNIT_TEST(CMWMC_Serialization)
{
  {
    CrossMwmConnectorBuilderEx<base::GeoObjectId> builder;
    // osmId featureId, segmentIdx, roadMask, oneWayMask, forwardIsEnter
    builder.AddTransition(base::MakeOsmWay(100ULL), 10, 1, kCarMask, kCarMask, true);
    builder.AddTransition(base::MakeOsmWay(200ULL), 20, 2, kCarMask, 0, true);
    builder.AddTransition(base::MakeOsmWay(300ULL), 30, 3, kPedestrianMask, kCarMask, true);
    TestSerialization(builder);
  }

  {
    CrossMwmConnectorBuilderEx<TransitId> builder;
    // osmId featureId, segmentIdx, roadMask, oneWayMask, forwardIsEnter
    builder.AddTransition(TransitId(1ULL /* stop 1 id */, 2ULL /* stop 2 id */, 1ULL /* line id */), 10, 1, kCarMask,
                          kCarMask, true);
    builder.AddTransition(TransitId(1ULL, 3ULL, 1ULL), 20, 2, kCarMask, 0, true);
    builder.AddTransition(TransitId(1ULL, 3ULL, 2ULL), 30, 3, kPedestrianMask, kCarMask, true);
    TestSerialization(builder);
  }
}

UNIT_TEST(CMWMC_WeightsSerialization)
{
  TestWeightsSerialization<base::GeoObjectId>();
  TestWeightsSerialization<TransitId>();
}
}  // namespace cross_mwm_connector_test
