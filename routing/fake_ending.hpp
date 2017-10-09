#pragma once

#include "routing/road_graph.hpp"
#include "routing/segment.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace routing
{
class WorldGraph;
class EdgeEstimator;
class IndexGraph;

struct Projection final
{
  Segment m_segment;
  bool m_isOneWay;
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
