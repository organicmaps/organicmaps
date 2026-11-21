#pragma once

#include "routing/latlon_with_altitude.hpp"
#include "routing/road_graph.hpp"
#include "routing/segment.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include <vector>

namespace routing
{
class EdgeEstimator;
class IndexGraph;
class WorldGraph;

struct Projection final
{
  Projection(Segment const & segment, bool isOneWay, LatLonWithAltitude const & segmentFront,
             LatLonWithAltitude const & segmentBack, LatLonWithAltitude const & junction)
    : m_segment(segment)
    , m_isOneWay(isOneWay)
    , m_segmentFront(segmentFront)
    , m_segmentBack(segmentBack)
    , m_junction(junction)
  {}
  bool operator==(Projection const & other) const;

  Segment m_segment;
  bool m_isOneWay = false;
  LatLonWithAltitude m_segmentFront;
  LatLonWithAltitude m_segmentBack;
  LatLonWithAltitude m_junction;
};

struct FakeEnding final
{
  LatLonWithAltitude m_originJunction;
  std::vector<Projection> m_projections;
};

template <typename Graph>
double GetSegmentLengthFraction(Graph & graph, Segment const & part, Segment const & whole)
{
  double const fullLength = ms::DistanceOnEarth(graph.GetPoint(whole, false), graph.GetPoint(whole, true));
  if (fullLength == 0.0)
    return 0.0;

  return ms::DistanceOnEarth(graph.GetPoint(part, false), graph.GetPoint(part, true)) / fullLength;
}

FakeEnding MakeFakeEnding(std::vector<Segment> const & segments, m2::PointD const & point, WorldGraph & graph);
FakeEnding MakeFakeEnding(Segment const & segment, m2::PointD const & point, IndexGraph & graph);

LatLonWithAltitude CalcProjectionToSegment(LatLonWithAltitude const & begin, LatLonWithAltitude const & end,
                                           m2::PointD const & point);
}  // namespace routing
