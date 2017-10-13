#pragma once

#include "routing/road_graph.hpp"
#include "routing/segment.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace routing
{
class EdgeEstimator;
class IndexGraph;
class WorldGraph;

struct Projection final
{
  Projection(Segment const & segment, bool isOneWay, Junction const & segmentFront,
             Junction const & segmentBack, Junction const & junction)
    : m_segment(segment)
    , m_isOneWay(isOneWay)
    , m_segmentFront(segmentFront)
    , m_segmentBack(segmentBack)
    , m_junction(junction)
  {
  }

  Segment m_segment;
  bool m_isOneWay = false;
  Junction m_segmentFront;
  Junction m_segmentBack;
  Junction m_junction;
};

struct FakeEnding final
{
  Junction m_originJunction;
  std::vector<Projection> m_projections;
};

FakeEnding MakeFakeEnding(Segment const & segment, m2::PointD const & point, WorldGraph & graph);
FakeEnding MakeFakeEnding(Segment const & segment, m2::PointD const & point,
                          EdgeEstimator const & estimator, IndexGraph & graph);
}  // namespace routing
