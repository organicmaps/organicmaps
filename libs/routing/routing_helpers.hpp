#pragma once

#include "routing/road_graph.hpp"
#include "routing/route.hpp"
#include "routing/route_weight.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "indexer/classificator.hpp"

#include "geometry/point_with_altitude.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/segment2d.hpp"

#include "base/cancellable.hpp"

#include <memory>
#include <queue>
#include <set>
#include <vector>

namespace routing
{
class DirectionsEngine;
class IndexGraphStarter;
class IndexRoadGraph;
class TrafficStash;
class WorldGraph;

template <typename Types>
bool IsCarRoad(Types const & types)
{
  return CarModel::AllLimitsInstance().HasRoadType(types);
}

template <typename Types>
bool IsBicycleRoad(Types const & types)
{
  return BicycleModel::AllLimitsInstance().HasRoadType(types);
}

/// \returns true when there exists a routing mode where the feature with |types| can be used.
template <typename Types>
bool IsRoad(Types const & types)
{
  if (IsCarRoad(types))
    return true;

  if (PedestrianModel::AllLimitsInstance().HasRoadType(types) || IsBicycleRoad(types))
  {
    // https://github.com/organicmaps/organicmaps/issues/3929
    // In case when road has pedestrian/bicycle=designated + highway=construction types.
    static uint32_t const constructionType = classif().GetTypeByPath({"highway", "construction"});
    return !base::IsExist(types, constructionType);
  }

  return false;
}

void FillSegmentInfo(std::vector<double> const & times, std::vector<RouteSegment> & routeSegments);

void ReconstructRoute(DirectionsEngine & engine, IndexRoadGraph const & graph, base::Cancellable const & cancellable,
                      std::vector<geometry::PointWithAltitude> const & path, std::vector<double> const & times,
                      Route & route);

/// \brief Converts |edge| to |segment|.
/// \returns Segment() if mwm of |edge| is not alive.
Segment ConvertEdgeToSegment(NumMwmIds const & numMwmIds, Edge const & edge);

/// \returns true if |segment| crosses any side of |rect|.
bool SegmentCrossesRect(m2::Segment2D const & segment, m2::RectD const & rect);

// \returns true if any part of polyline |junctions| lay in |rect| and false otherwise.
bool RectCoversPolyline(IRoadGraph::PointWithAltitudeVec const & junctions, m2::RectD const & rect);

/// \brief Checks is edge connected with world graph. Function does BFS while it finds some number
/// of edges,
/// if graph ends before this number is reached then junction is assumed as not connected to the
/// world graph.
bool CheckGraphConnectivity(Segment const & start, bool isOutgoing, bool useRoutingOptions, size_t limit,
                            WorldGraph & graph, std::set<Segment> & marked);

struct AStarLengthChecker
{
  explicit AStarLengthChecker(IndexGraphStarter & starter);
  bool operator()(RouteWeight const & weight) const;
  IndexGraphStarter & m_starter;
};

struct AdjustLengthChecker
{
  explicit AdjustLengthChecker(IndexGraphStarter & starter);
  bool operator()(RouteWeight const & weight) const;
  IndexGraphStarter & m_starter;
};
}  // namespace routing
