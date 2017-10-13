#include "routing/fake_ending.hpp"

#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/world_graph.hpp"

#include "geometry/distance.hpp"

#include "base/math.hpp"

#include <utility>

using namespace routing;
using namespace std;

namespace
{
template <typename Estimator>
Junction CalcProjectionToSegment(Junction const & begin, Junction const & end,
                                 m2::PointD const & point, Estimator && estimator)
{
  m2::ProjectionToSection<m2::PointD> projection;

  projection.SetBounds(begin.GetPoint(), end.GetPoint());
  auto const projectedPoint = projection(point);
  auto const distBeginToEnd = estimator(begin.GetPoint(), end.GetPoint());

  double constexpr kEpsMeters = 2.0;
  if (my::AlmostEqualAbs(distBeginToEnd, 0.0, kEpsMeters))
    return Junction(projectedPoint, begin.GetAltitude());

  auto const distBeginToProjection = estimator(begin.GetPoint(), projectedPoint);
  auto const altitude = begin.GetAltitude() + (end.GetAltitude() - begin.GetAltitude()) *
                                                  distBeginToProjection / distBeginToEnd;
  return Junction(projectedPoint, altitude);
}

template <typename Estimator>
FakeEnding MakeFakeEndingImpl(Junction const & backJunction, Junction const & frontJunction,
                              Segment const & segment, m2::PointD const & point, bool oneWay,
                              Estimator && estimator)
{
  auto const & projectedJunction =
      CalcProjectionToSegment(backJunction, frontJunction, point, forward<Estimator>(estimator));

  FakeEnding ending;
  ending.m_originJunction = Junction(point, projectedJunction.GetAltitude());
  ending.m_projections.emplace_back(segment, oneWay, frontJunction, backJunction,
                                    projectedJunction);
  return ending;
}
}  // namespace

namespace routing
{
FakeEnding MakeFakeEnding(Segment const & segment, m2::PointD const & point,
                          EdgeEstimator const & estimator, IndexGraph & graph)
{
  auto const & road = graph.GetGeometry().GetRoad(segment.GetFeatureId());
  bool const oneWay = road.IsOneWay();
  auto const & frontJunction = road.GetJunction(segment.GetPointId(true /* front */));
  auto const & backJunction = road.GetJunction(segment.GetPointId(false /* front */));
  return MakeFakeEndingImpl(backJunction, frontJunction, segment, point, oneWay,
                            [&](m2::PointD const & p1, m2::PointD const & p2) {
                              return estimator.CalcLeapWeight(p1, p2);
                            });
}

FakeEnding MakeFakeEnding(Segment const & segment, m2::PointD const & point, WorldGraph & graph)
{
  bool const oneWay = graph.IsOneWay(segment.GetMwmId(), segment.GetFeatureId());
  auto const & frontJunction = graph.GetJunction(segment, true /* front */);
  auto const & backJunction = graph.GetJunction(segment, false /* front */);
  return MakeFakeEndingImpl(backJunction, frontJunction, segment, point, oneWay,
                            [&](m2::PointD const & p1, m2::PointD const & p2) {
                              return graph.CalcLeapWeight(p1, p2).GetWeight();
                            });
}
}  // namespace routing
