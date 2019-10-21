#include "routing/fake_ending.hpp"

#include "routing/index_graph.hpp"
#include "routing/world_graph.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"

#include "base/math.hpp"

#include <utility>

using namespace routing;
using namespace std;

namespace
{
Junction CalcProjectionToSegment(Junction const & begin, Junction const & end,
                                 m2::PointD const & point)
{
  m2::ParametrizedSegment<m2::PointD> segment(begin.GetPoint(), end.GetPoint());

  auto const projectedPoint = segment.ClosestPointTo(point);
  auto const distBeginToEnd = mercator::DistanceOnEarth(begin.GetPoint(), end.GetPoint());

  double constexpr kEpsMeters = 2.0;
  if (base::AlmostEqualAbs(distBeginToEnd, 0.0, kEpsMeters))
    return Junction(projectedPoint, begin.GetAltitude());

  auto const distBeginToProjection = mercator::DistanceOnEarth(begin.GetPoint(), projectedPoint);
  auto const altitude = begin.GetAltitude() + (end.GetAltitude() - begin.GetAltitude()) *
                                                  distBeginToProjection / distBeginToEnd;
  return Junction(projectedPoint, altitude);
}
}  // namespace

namespace routing
{
FakeEnding MakeFakeEnding(vector<Segment> const & segments, m2::PointD const & point,
                          WorldGraph & graph)
{
  FakeEnding ending;
  double averageAltitude = 0.0;

  for (size_t i = 0; i < segments.size(); ++i)
  {
    auto const & segment = segments[i];

    bool const oneWay = graph.IsOneWay(segment.GetMwmId(), segment.GetFeatureId());
    auto const & frontJunction = graph.GetJunction(segment, true /* front */);
    auto const & backJunction = graph.GetJunction(segment, false /* front */);
    auto const & projectedJunction = CalcProjectionToSegment(backJunction, frontJunction, point);

    ending.m_projections.emplace_back(segment, oneWay, frontJunction, backJunction,
                                      projectedJunction);

    averageAltitude = (i * averageAltitude + projectedJunction.GetAltitude()) / (i + 1);
  }

  ending.m_originJunction = Junction(point, static_cast<feature::TAltitude>(averageAltitude));
  return ending;
}

FakeEnding MakeFakeEnding(Segment const & segment, m2::PointD const & point, IndexGraph & graph)
{
  auto const & road = graph.GetGeometry().GetRoad(segment.GetFeatureId());
  bool const oneWay = road.IsOneWay();
  auto const & frontJunction = road.GetJunction(segment.GetPointId(true /* front */));
  auto const & backJunction = road.GetJunction(segment.GetPointId(false /* front */));
  auto const & projectedJunction = CalcProjectionToSegment(backJunction, frontJunction, point);

  FakeEnding ending;
  ending.m_originJunction = Junction(point, projectedJunction.GetAltitude());
  ending.m_projections.emplace_back(segment, oneWay, frontJunction, backJunction,
                                    projectedJunction);
  return ending;
}
}  // namespace routing
