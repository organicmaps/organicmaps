#include "testing/testing.hpp"

#include "transit/transit_tests/transit_tools.hpp"

#include "transit/experimental/transit_data.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace transit
{
namespace experimental
{
enum class TransitUseCase
{
  AllData,
  Routing,
  Rendering,
  CrossMwm
};

void TestEqual(TransitData const & actualTransit, TransitData const & expectedTransit,
               TransitUseCase const & useCase)
{
  switch (useCase)
  {
  case TransitUseCase::AllData:
    TestEqual(actualTransit.GetStops(), expectedTransit.GetStops());
    TestEqual(actualTransit.GetGates(), expectedTransit.GetGates());
    TestEqual(actualTransit.GetEdges(), expectedTransit.GetEdges());
    TestEqual(actualTransit.GetTransfers(), expectedTransit.GetTransfers());
    TestEqual(actualTransit.GetLines(), expectedTransit.GetLines());
    TestEqual(actualTransit.GetLinesMetadata(), expectedTransit.GetLinesMetadata());
    TestEqual(actualTransit.GetShapes(), expectedTransit.GetShapes());
    TestEqual(actualTransit.GetRoutes(), expectedTransit.GetRoutes());
    TestEqual(actualTransit.GetNetworks(), expectedTransit.GetNetworks());
    break;

  case TransitUseCase::Routing:
    TestEqual(actualTransit.GetStops(), expectedTransit.GetStops());
    TestEqual(actualTransit.GetGates(), expectedTransit.GetGates());
    TestEqual(actualTransit.GetEdges(), expectedTransit.GetEdges());
    TEST(actualTransit.GetTransfers().empty(), ());
    TestEqual(actualTransit.GetLines(), expectedTransit.GetLines());
    TEST(actualTransit.GetShapes().empty(), ());
    TEST(actualTransit.GetRoutes().empty(), ());
    TEST(actualTransit.GetNetworks().empty(), ());
    break;

  case TransitUseCase::Rendering:
    TestEqual(actualTransit.GetStops(), expectedTransit.GetStops());
    TEST(actualTransit.GetGates().empty(), ());
    TestEqual(actualTransit.GetEdges(), expectedTransit.GetEdges());
    TestEqual(actualTransit.GetTransfers(), expectedTransit.GetTransfers());
    TestEqual(actualTransit.GetLines(), expectedTransit.GetLines());
    TestEqual(actualTransit.GetLinesMetadata(), expectedTransit.GetLinesMetadata());
    TestEqual(actualTransit.GetShapes(), expectedTransit.GetShapes());
    TestEqual(actualTransit.GetRoutes(), expectedTransit.GetRoutes());
    TEST(actualTransit.GetNetworks().empty(), ());
    break;

  case TransitUseCase::CrossMwm:
    TestEqual(actualTransit.GetStops(), expectedTransit.GetStops());
    TEST(actualTransit.GetGates().empty(), ());
    TestEqual(actualTransit.GetEdges(), expectedTransit.GetEdges());
    TEST(actualTransit.GetTransfers().empty(), ());
    TEST(actualTransit.GetLines().empty(), ());
    TEST(actualTransit.GetShapes().empty(), ());
    TEST(actualTransit.GetRoutes().empty(), ());
    TEST(actualTransit.GetNetworks().empty(), ());
    break;

  default: UNREACHABLE();
  }
}

void SerDesTransit(TransitData & src, TransitData & dst, TransitUseCase const & useCase)
{
  std::vector<uint8_t> buffer;
  MemWriter<decltype(buffer)> writer(buffer);
  src.Serialize(writer);

  MemReader reader(buffer.data(), buffer.size());
  switch (useCase)
  {
  case TransitUseCase::AllData: dst.Deserialize(reader); break;
  case TransitUseCase::Routing: dst.DeserializeForRouting(reader); break;
  case TransitUseCase::Rendering: dst.DeserializeForRendering(reader); break;
  case TransitUseCase::CrossMwm: dst.DeserializeForCrossMwm(reader); break;
  default: UNREACHABLE();
  }

  dst.CheckValid();
}

TransitData FillTestTransitData()
{
  TransitData data;
  data.m_networks = {
      Network(4032061671 /* transitId */, "ГУП МосГорТранс" /* title */),
      Network(4035419440 /* transitId */, "EMPRESA DE TRANSPORTE DEL SUR SRL" /* title */),
      Network(4035418196 /* transitId */, "Buslink Sunraysia" /* title */)};

  data.m_routes = {Route(4036206872 /* id */, 4035419440 /* networkId */, "bus" /* routeType */,
                         "Echuca/Moama - Melbourne Via Shepparton" /* title */, "gray" /* color */),
                   Route(4027700598 /* id */, 4035418196 /* networkId */, "ferry" /* routeType */,
                         "Mount Beauty - Melbourne Via Brigh" /* title */, "purple" /* color */),
                   Route(4027700599 /* id */, 4032061671 /* networkId */, "ferry" /* routeType */,
                         "Киевский вокзал - парк Зарядье" /* title */, "purple" /* color */)};

  FrequencyIntervals frequencies;
  frequencies.AddInterval(TimeInterval(6530334760), 3000);

  Schedule schedule;
  schedule.SetDefaultFrequency(4060);

  schedule.AddDatesInterval(DatesInterval(75007489), frequencies);

  data.m_lines = {
      Line(4036598626 /* id */, 4036206872 /* routeId */,
           ShapeLink(4036591460 /* id */, 0 /* startIndex */, 2690 /* endIndex */),
           "740G" /* title */,
           IdList{4036592571, 4036592572, 4036592573, 4036592574, 4036592575, 4036592576},
           schedule),
      Line(
          4036598627 /* id */, 4036206872 /* routeId */,
          ShapeLink(4036591461 /* id */, 356 /* startIndex */, 40690 /* endIndex */),
          "" /* title */,
          IdList{4027013783, 4027013784, 4027013785, 4027013786, 4027013787, 4027013788, 4027013789,
                 4027013790, 4027013791, 4027013792, 4027013793, 4027013794, 4027013795, 4027013796,
                 4027013797, 4027013798, 4027013799, 4027013800, 4027013801},
          schedule)};

  data.m_linesMetadata = {
      LineMetadata(4036598626 /* id */, LineSegmentsOrder{LineSegmentOrder({0, 100}, 0),
                                                          LineSegmentOrder({100, 205}, 2)}),
      LineMetadata(4036598627 /* id */, LineSegmentsOrder{})};

  data.m_stops = {Stop(4026990853 /* id */, kInvalidFeatureId /* featureId */,
                       kInvalidOsmId /* osmId */, "CARLOS DIHEL 2500-2598" /* title */,
                       TimeTable{{204, std::vector<TimeInterval>{TimeInterval(11400205248)}}},
                       m2::PointD(-58.57196, -36.82596), {} /* transferIds */),
                  Stop(4026990854 /* id */, kInvalidFeatureId /* featureId */,
                       kInvalidOsmId /* osmId */, "QUIROGA 1901-1999" /* title */, TimeTable{},
                       m2::PointD(-58.57196, -36.82967), {} /* transferIds */)};

  data.SetStopPedestrianSegments(
      0 /* stopIdx */,
      {SingleMwmSegment(981 /* featureId */, 0 /* segmentIdx */, true /* forward */),
       SingleMwmSegment(91762 /* featureId */, 108 /* segmentIdx */, false /* forward */)});
  data.SetStopPedestrianSegments(
      1 /* stopIdx */,
      {SingleMwmSegment(15000 /* featureId */, 100 /* segmentIdx */, false /* forward */)});

  data.m_gates = {Gate(4034666808 /* id */, kInvalidFeatureId /* featureId */,
                       kInvalidOsmId /* osmId */, true /* entrance */, true /* exit */,
                       std::vector<TimeFromGateToStop>{
                           TimeFromGateToStop(4026990854 /* stopId */, 76 /* timeSeconds */)},
                       m2::PointD(-121.84063, 40.19393)),
                  Gate(4034666809 /* id */, kInvalidFeatureId /* featureId */,
                       kInvalidOsmId /* osmId */, false /* entrance */, true /* exit */,
                       std::vector<TimeFromGateToStop>{
                           TimeFromGateToStop(4026990857 /* stopId */, 0 /* timeSeconds */),
                           TimeFromGateToStop(4026990858 /* stopId */, 0 /* timeSeconds */)},
                       m2::PointD(-58.96030, -36.40335)),
                  Gate(4034666810 /* id */, kInvalidFeatureId /* featureId */,
                       kInvalidOsmId /* osmId */, true /* entrance */, false /* exit */,
                       std::vector<TimeFromGateToStop>{
                           TimeFromGateToStop(4026990889 /* stopId */, 80 /* timeSeconds */),
                           TimeFromGateToStop(4026990890 /* stopId */, 108 /* timeSeconds */),
                           TimeFromGateToStop(4026990891 /* stopId */, 90 /* timeSeconds */)},
                       m2::PointD(-58.96030, -36.40335))};

  data.SetGatePedestrianSegments(
      1 /* gateIdx */,
      {SingleMwmSegment(6861 /* featureId */, 15 /* segmentIdx */, true /* forward */),
       SingleMwmSegment(307920 /* featureId */, 2 /* segmentIdx */, false /* forward */)});

  data.m_edges = {
      Edge(4036592295 /* stop1Id */, 4036592296 /* stop2Id */, 69 /* weight */,
           4036591958 /* lineId */, false /* transfer */,
           ShapeLink(4036591484 /* shapeId */, 592 /* startIndex */, 609 /* endIndex */)),
      Edge(4030648032 /* stop1Id */, 4030648073 /* stop2Id */, 40 /* weight */,
           kInvalidTransitId /* lineId */, true /* transfer */,
           ShapeLink(kInvalidTransitId /* shapeId */, 0 /* startIndex */, 0 /* endIndex */))};

  data.m_transfers = {Transfer(4029752024 /* id */, m2::PointD(-122.16915, 40.41578) /* point */,
                               IdList{4029751945, 4029752010} /* stopIds */)};

  return data;
}

UNIT_TEST(SerDes_All)
{
  TransitData plan = FillTestTransitData();
  TransitData fact;
  SerDesTransit(plan, fact, TransitUseCase::AllData);
  TestEqual(fact, plan, TransitUseCase::AllData);
}

UNIT_TEST(SerDes_ForRouting)
{
  TransitData plan = FillTestTransitData();
  TransitData fact;
  SerDesTransit(plan, fact, TransitUseCase::Routing);
  TestEqual(fact, plan, TransitUseCase::Routing);
}

UNIT_TEST(SerDes_ForRendering)
{
  TransitData plan = FillTestTransitData();
  TransitData fact;
  SerDesTransit(plan, fact, TransitUseCase::Rendering);
  TestEqual(fact, plan, TransitUseCase::Rendering);
}

UNIT_TEST(SerDes_ForCrossMwm)
{
  TransitData plan = FillTestTransitData();
  TransitData fact;
  SerDesTransit(plan, fact, TransitUseCase::CrossMwm);
  TestEqual(fact, plan, TransitUseCase::CrossMwm);
}
}  // namespace experimental
}  // namespace transit
