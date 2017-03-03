#include "testing/testing.hpp"

#include "routing/cross_mwm_ramp_serialization.hpp"

#include "coding/writer.hpp"

using namespace routing;
using namespace std;

namespace
{
NumMwmId constexpr mwmId = 777;

void CheckRampConsistency(CrossMwmRamp const & ramp)
{
  for (Segment const & enter : ramp.GetEnters())
  {
    TEST(ramp.IsTransition(enter, true /* isOutgoing */), ());
    TEST(!ramp.IsTransition(enter, false /* isOutgoing */), ());
  }

  for (Segment const & exit : ramp.GetExits())
  {
    TEST(!ramp.IsTransition(exit, true /* isOutgoing */), ());
    TEST(ramp.IsTransition(exit, false /* isOutgoing */), ());
  }
}

void CheckEdges(CrossMwmRamp const & ramp, Segment const & from, bool isOutgoing,
                vector<SegmentEdge> const & expectedEdges)
{
  vector<SegmentEdge> edges;
  ramp.GetEdgeList(from, isOutgoing, edges);
  TEST_EQUAL(edges, expectedEdges, ());
}
}

namespace routing_test
{
UNIT_TEST(OneWayEnter)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  CrossMwmRamp ramp(mwmId);
  ramp.AddTransition(featureId, segmentIdx, true /* oneWay */, true /* forwardIsEnter */,
                     {} /* backPoint */, {} /* frontPoint */);

  CheckRampConsistency(ramp);
  TEST_EQUAL(ramp.GetEnters().size(), 1, ());
  TEST_EQUAL(ramp.GetExits().size(), 0, ());
  TEST(ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                         true /* isOutgoing */),
       ());
  TEST(!ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                          false /* isOutgoing */),
       ());
  TEST(!ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                          true /* isOutgoing */),
       ());
  TEST(!ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                          false /* isOutgoing */),
       ());
}

UNIT_TEST(OneWayExit)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  CrossMwmRamp ramp(mwmId);
  ramp.AddTransition(featureId, segmentIdx, true /* oneWay */, false /* forwardIsEnter */,
                     {} /* backPoint */, {} /* frontPoint */);

  CheckRampConsistency(ramp);
  TEST_EQUAL(ramp.GetEnters().size(), 0, ());
  TEST_EQUAL(ramp.GetExits().size(), 1, ());
  TEST(!ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                          true /* isOutgoing */),
       ());
  TEST(ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                         false /* isOutgoing */),
       ());
  TEST(!ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                          true /* isOutgoing */),
       ());
  TEST(!ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                          false /* isOutgoing */),
       ());
}

UNIT_TEST(TwoWayEnter)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  CrossMwmRamp ramp(mwmId);
  ramp.AddTransition(featureId, segmentIdx, false /* oneWay */, true /* forwardIsEnter */,
                     {} /* backPoint */, {} /* frontPoint */);

  CheckRampConsistency(ramp);
  TEST_EQUAL(ramp.GetEnters().size(), 1, ());
  TEST_EQUAL(ramp.GetExits().size(), 1, ());
  TEST(ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                         true /* isOutgoing */),
       ());
  TEST(!ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                          false /* isOutgoing */),
       ());
  TEST(!ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                          true /* isOutgoing */),
       ());
  TEST(ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                         false /* isOutgoing */),
       ());
}

UNIT_TEST(TwoWayExit)
{
  uint32_t constexpr featureId = 1;
  uint32_t constexpr segmentIdx = 1;
  CrossMwmRamp ramp(mwmId);
  ramp.AddTransition(featureId, segmentIdx, false /* oneWay */, false /* forwardIsEnter */,
                     {} /* backPoint */, {} /* frontPoint */);

  CheckRampConsistency(ramp);
  TEST_EQUAL(ramp.GetEnters().size(), 1, ());
  TEST_EQUAL(ramp.GetExits().size(), 1, ());
  TEST(!ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                          true /* isOutgoing */),
       ());
  TEST(ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, true /* forward */),
                         false /* isOutgoing */),
       ());
  TEST(ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                         true /* isOutgoing */),
       ());
  TEST(!ramp.IsTransition(Segment(mwmId, featureId, segmentIdx, false /* forward */),
                          false /* isOutgoing */),
       ());
}

UNIT_TEST(Serialization)
{
  float constexpr kEdgesWeight = 333;

  vector<uint8_t> buffer;
  {
    vector<CrossMwmRampSerializer::Transition> transitions = {
        /* featureId, segmentIdx, roadMask, oneWayMask, forwardIsEnter, backPoint, frontPoint */
        {10, 1, kCarMask, kCarMask, true, m2::PointD(1.1, 1.2), m2::PointD(1.3, 1.4)},
        {20, 2, kCarMask, 0, true, m2::PointD(2.1, 2.2), m2::PointD(2.3, 2.4)},
        {30, 3, kPedestrianMask, kCarMask, true, m2::PointD(3.1, 3.2), m2::PointD(3.3, 3.4)}};

    vector<CrossMwmRamp> ramps(static_cast<size_t>(VehicleType::Count), mwmId);

    CrossMwmRamp & carRamp = ramps[static_cast<size_t>(VehicleType::Car)];
    for (auto const & transition : transitions)
      CrossMwmRampSerializer::AddTransition(transition, kCarMask, carRamp);

    carRamp.FillWeights([](Segment const & enter, Segment const & exit) { return kEdgesWeight; });

    serial::CodingParams const codingParams;
    MemWriter<vector<uint8_t>> writer(buffer);
    CrossMwmRampSerializer::Serialize(transitions, ramps, codingParams, writer);
  }

  CrossMwmRamp ramp(mwmId);
  {
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> source(reader);
    CrossMwmRampSerializer::DeserializeTransitions(VehicleType::Car, ramp, source);
  }

  CheckRampConsistency(ramp);

  TEST_EQUAL(ramp.GetEnters().size(), 2, ());
  TEST_EQUAL(ramp.GetExits().size(), 1, ());

  TEST(!ramp.IsTransition(Segment(mwmId, 0, 0, true), true /* isOutgoing */), ());

  TEST(ramp.IsTransition(Segment(mwmId, 10, 1, true /* forward */), true /* isOutgoing */), ());
  TEST(!ramp.IsTransition(Segment(mwmId, 10, 1, true /* forward */), false /* isOutgoing */), ());
  TEST(!ramp.IsTransition(Segment(mwmId, 10, 1, false /* forward */), true /* isOutgoing */), ());
  TEST(!ramp.IsTransition(Segment(mwmId, 10, 1, false /* forward */), false /* isOutgoing */), ());

  TEST(ramp.IsTransition(Segment(mwmId, 20, 2, true /* forward */), true /* isOutgoing */), ());
  TEST(!ramp.IsTransition(Segment(mwmId, 20, 2, true /* forward */), false /* isOutgoing */), ());
  TEST(!ramp.IsTransition(Segment(mwmId, 20, 2, false /* forward */), true /* isOutgoing */), ());
  TEST(ramp.IsTransition(Segment(mwmId, 20, 2, false /* forward */), false /* isOutgoing */), ());

  TEST(!ramp.IsTransition(Segment(mwmId, 30, 3, true /* forward */), true /* isOutgoing */), ());

  TEST(!ramp.WeightsWereLoaded(), ());
  TEST(!ramp.HasWeights(), ());

  {
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> source(reader);
    CrossMwmRampSerializer::DeserializeWeights(VehicleType::Car, ramp, source);
  }
  TEST(ramp.WeightsWereLoaded(), ());
  TEST(ramp.HasWeights(), ());

  double constexpr eps = 1e-6;
  TEST(AlmostEqualAbs(ramp.GetPoint(Segment(mwmId, 20, 2, true /* forward */), true /* front */),
                      m2::PointD(2.3, 2.4), eps),
       ());
  TEST(AlmostEqualAbs(ramp.GetPoint(Segment(mwmId, 20, 2, true /* forward */), false /* front */),
                      m2::PointD(2.1, 2.2), eps),
       ());
  TEST(AlmostEqualAbs(ramp.GetPoint(Segment(mwmId, 20, 2, false /* forward */), true /* front */),
                      m2::PointD(2.1, 2.2), eps),
       ());
  TEST(AlmostEqualAbs(ramp.GetPoint(Segment(mwmId, 20, 2, true /* forward */), true /* front */),
                      m2::PointD(2.3, 2.4), eps),
       ());

  CheckEdges(ramp, Segment(mwmId, 10, 1, true /* forward */), true /* isOutgoing */,
             {{Segment(mwmId, 20, 2, false /* forward */), kEdgesWeight}});

  CheckEdges(ramp, Segment(mwmId, 20, 2, true /* forward */), true /* isOutgoing */,
             {{Segment(mwmId, 20, 2, false /* forward */), kEdgesWeight}});

  CheckEdges(ramp, Segment(mwmId, 20, 2, false /* forward */), false /* isOutgoing */,
             {{Segment(mwmId, 10, 1, true /* forward */), kEdgesWeight},
              {Segment(mwmId, 20, 2, true /* forward */), kEdgesWeight}});
}
}  // namespace routing_test
