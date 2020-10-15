#include "routing/pedestrian_directions.hpp"

#include "routing/road_graph.hpp"
#include "routing/turns_generator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <utility>

using namespace std;

namespace routing
{
PedestrianDirectionsEngine::PedestrianDirectionsEngine(DataSource const & dataSource,
                                                       shared_ptr<NumMwmIds> numMwmIds)
  : DirectionsEngine(dataSource, move(numMwmIds))
{
}

bool PedestrianDirectionsEngine::Generate(IndexRoadGraph const & graph,
                                          vector<geometry::PointWithAltitude> const & path,
                                          base::Cancellable const & cancellable,
                                          Route::TTurns & turns, Route::TStreets & streetNames,
                                          vector<geometry::PointWithAltitude> & routeGeometry,
                                          vector<Segment> & segments)
{
  CHECK(m_numMwmIds, ());

  m_adjacentEdges.clear();
  m_pathSegments.clear();
  turns.clear();
  streetNames.clear();
  segments.clear();

  vector<Edge> routeEdges;

  CHECK_NOT_EQUAL(m_vehicleType, VehicleType::Count, (m_vehicleType));

  if (m_vehicleType == VehicleType::Transit)
  {
    routeGeometry = path;
    graph.GetRouteSegments(segments);
    graph.GetRouteEdges(routeEdges);
    turns.emplace_back(routeEdges.size(), turns::PedestrianDirection::ReachedYourDestination);
    return true;
  }

  routeGeometry.clear();

  if (path.size() <= 1)
    return false;

  size_t const pathSize = path.size();

  graph.GetRouteEdges(routeEdges);

  if (routeEdges.empty())
    return false;

  if (cancellable.IsCancelled())
    return false;

  FillPathSegmentsAndAdjacentEdgesMap(graph, path, routeEdges, cancellable);

  if (cancellable.IsCancelled())
    return false;

  RoutingEngineResult resultGraph(routeEdges, m_adjacentEdges, m_pathSegments);
  auto const res =
      MakeTurnAnnotationPedestrian(resultGraph, *m_numMwmIds, m_vehicleType, cancellable,
                                   routeGeometry, turns, streetNames, segments);

  if (res != RouterResultCode::NoError)
    return false;

  CHECK_EQUAL(
      routeGeometry.size(), pathSize,
      ("routeGeometry and path have different sizes. routeGeometry size:", routeGeometry.size(),
       "path size:", pathSize, "segments size:", segments.size(), "routeEdges size:",
       routeEdges.size(), "resultGraph.GetSegments() size:", resultGraph.GetSegments().size()));

  return true;
}
}  // namespace routing
