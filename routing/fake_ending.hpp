#pragma once

#include "routing/road_graph.hpp"
#include "routing/segment.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include <vector>

namespace routing
{
class EdgeEstimator;
class IndexGraph;
class WorldGraph;

struct Projection final
{
  Projection(Segment const & segment, bool isOneWay,
             geometry::PointWithAltitude const & segmentFront,
             geometry::PointWithAltitude const & segmentBack,
             geometry::PointWithAltitude const & junction)
    : m_segment(segment)
    , m_isOneWay(isOneWay)
    , m_segmentFront(segmentFront)
    , m_segmentBack(segmentBack)
    , m_junction(junction)
  {
  }

  Segment m_segment;
  bool m_isOneWay = false;
  geometry::PointWithAltitude m_segmentFront;
  geometry::PointWithAltitude m_segmentBack;
  geometry::PointWithAltitude m_junction;
};

struct FakeEnding final
{
  geometry::PointWithAltitude m_originJunction;
  std::vector<Projection> m_projections;
};

FakeEnding MakeFakeEnding(std::vector<Segment> const & segments, m2::PointD const & point,
                          WorldGraph & graph);
FakeEnding MakeFakeEnding(Segment const & segment, m2::PointD const & point, IndexGraph & graph);
}  // namespace routing
