#include "routing/routing_helpers.hpp"
#include "routing/road_point.hpp"

#include "traffic/traffic_info.hpp"

namespace routing
{
using namespace traffic;

void ReconstructRoute(IDirectionsEngine * engine, IRoadGraph const & graph,
                      shared_ptr<TrafficInfo::Coloring> trafficColoring,
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
  vector<TrafficInfo::RoadSegmentId> routeSegs;
  if (engine)
    engine->Generate(graph, path, times, turnsDir, junctions, routeSegs, cancellable);

  vector<m2::PointD> routeGeometry;
  JunctionsToPoints(junctions, routeGeometry);
  feature::TAltitudes altitudes;
  JunctionsToAltitudes(junctions, altitudes);

  route.SetGeometry(routeGeometry.begin(), routeGeometry.end());
  route.SetSectionTimes(move(times));
  route.SetTurnInstructions(move(turnsDir));
  route.SetStreetNames(move(streetNames));
  route.SetAltitudes(move(altitudes));

  vector<traffic::SpeedGroup> traffic;
  traffic.reserve(routeSegs.size());
  if (trafficColoring)
  {
    for (TrafficInfo::RoadSegmentId const & rp : routeSegs)
    {
      auto const it = trafficColoring->find(rp);
      SpeedGroup segTraffic =  (it == trafficColoring->cend()) ? SpeedGroup::Unknown
                                                               : it->second;
      // @TODO It's written to compensate an error. The problem is in case of any routing except for osrm
      // all route points except for begining and ending are duplicated.
      // See a TODO in BicycleDirectionsEngine::Generate() for details.
      // At this moment the route looks like:
      // 0----1    4----5
      //      2----3    6---7
      // This route composes of 4 edges and there're 4 items in routeSegs.
      // But it has 8 items in routeGeometry.
      // So for segments 0-1 and 1-2 let's set routeSegs[0]
      // for segments 2-3 and 3-4 let's set routeSegs[1]
      // for segments 4-5 and 5-7 let's set routeSegs[2]
      // and for segment 6-7 let's set routeSegs[3]
      traffic.insert(traffic.end(), {segTraffic, segTraffic});
    }
    if (!traffic.empty())
      traffic.pop_back();
  }

  route.SetTraffic(move(traffic));
}
}  // namespace rouing
