#include "routing/routing_helpers.hpp"

namespace routing
{
void ReconstructRoute(IDirectionsEngine * engine, IRoadGraph const & graph,
                      my::Cancellable const & cancellable, vector<Junction> & path, Route & route)
{
  if (path.empty())
  {
    LOG(LERROR, ("Can't reconstruct route from an empty list of positions."));
    return;
  }

  // By some reason there're two adjacent positions on a road with
  // the same end-points. This could happen, for example, when
  // direction on a road was changed.  But it doesn't matter since
  // this code reconstructs only geometry of a route.
  path.erase(unique(path.begin(), path.end()), path.end());

  Route::TTimes times;
  Route::TTurns turnsDir;
  vector<Junction> junctions;
  // @TODO(bykoianko) streetNames is not filled in Generate(). It should be done.
  Route::TStreets streetNames;
  if (engine)
    engine->Generate(graph, path, times, turnsDir, junctions, cancellable);

  vector<m2::PointD> routeGeometry;
  JunctionsToPoints(junctions, routeGeometry);
  feature::TAltitudes altitudes;
  JunctionsToAltitudes(junctions, altitudes);

  route.SetGeometry(routeGeometry.begin(), routeGeometry.end());
  route.SetSectionTimes(move(times));
  route.SetTurnInstructions(move(turnsDir));
  route.SetStreetNames(move(streetNames));
  route.SetAltitudes(move(altitudes));
}
}  // namespace rouing
