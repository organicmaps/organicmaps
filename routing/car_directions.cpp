#include "routing/car_directions.hpp"

#include "routing/fake_feature_ids.hpp"
#include "routing/road_point.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turns.hpp"
#include "routing/turns_generator.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace routing
{
using namespace std;

CarDirectionsEngine::CarDirectionsEngine(DataSource const & dataSource,
                                         shared_ptr<NumMwmIds> numMwmIds)
  : DirectionsEngine(dataSource, move(numMwmIds))
{
}

bool CarDirectionsEngine::Generate(IndexRoadGraph const & graph,
                                   vector<geometry::PointWithAltitude> const & path,
                                   base::Cancellable const & cancellable, Route::TTurns & turns,
                                   Route::TStreets & streetNames,
                                   vector<geometry::PointWithAltitude> & routeGeometry,
                                   vector<Segment> & segments)
{
  CHECK(m_numMwmIds, ());

  m_adjacentEdges.clear();
  m_pathSegments.clear();
  turns.clear();
  streetNames.clear();
  routeGeometry.clear();
  segments.clear();

  size_t const pathSize = path.size();
  // Note. According to Route::IsValid() method route of zero or one point is invalid.
  if (pathSize <= 1)
    return false;

  IRoadGraph::EdgeVector routeEdges;
  graph.GetRouteEdges(routeEdges);

  if (routeEdges.empty())
    return false;

  if (cancellable.IsCancelled())
    return false;

  FillPathSegmentsAndAdjacentEdgesMap(graph, path, routeEdges, cancellable);

  if (cancellable.IsCancelled())
    return false;

  RoutingEngineResult resultGraph(routeEdges, m_adjacentEdges, m_pathSegments);
  CHECK_NOT_EQUAL(m_vehicleType, VehicleType::Count, (m_vehicleType));

  auto const res = MakeTurnAnnotation(resultGraph, *m_numMwmIds, m_vehicleType, cancellable,
                                      routeGeometry, turns, streetNames, segments);

  if (res != RouterResultCode::NoError)
    return false;

  CHECK_EQUAL(
      routeGeometry.size(), pathSize,
      ("routeGeometry and path have different sizes. routeGeometry size:", routeGeometry.size(),
       "path size:", pathSize, "segments size:", segments.size(), "routeEdges size:",
       routeEdges.size(), "resultGraph.GetSegments() size:", resultGraph.GetSegments().size()));

  // In case of bicycle routing |m_pathSegments| may have an empty
  // |LoadedPathSegment::m_segments| fields. In that case |segments| is empty
  // so size of |segments| is not equal to size of |routeEdges|.
  if (!segments.empty())
    CHECK_EQUAL(segments.size(), routeEdges.size(), ());
  return true;
}
}  // namespace routing
