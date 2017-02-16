#include "routing/routing_helpers.hpp"
#include "routing/road_point.hpp"

#include "traffic/traffic_info.hpp"

namespace routing
{
using namespace traffic;

void ReconstructRoute(IDirectionsEngine & engine, RoadGraphBase const & graph,
                      shared_ptr<TrafficStash> const & trafficStash,
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
  vector<Segment> trafficSegs;
  engine.Generate(graph, path, cancellable, times, turnsDir, junctions, trafficSegs);

  if (cancellable.IsCancelled())
    return;

  // In case of any errors in IDirectionsEngine::Generate() |junctions| is empty.
  if (junctions.empty())
  {
    LOG(LERROR, ("Internal error happened while turn generation."));
    return;
  }

  // @TODO(bykoianko) If the start and the finish of a route lies on the same road segment
  // engine->Generate() fills with empty vectors |times|, |turnsDir|, |junctions| and |trafficSegs|.
  // It's not correct and should be fixed. It's necessary to work corrrectly with such routes.

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
  if (trafficStash && !trafficSegs.empty())
  {
    traffic.reserve(2 * trafficSegs.size());
    for (Segment const & seg : trafficSegs)
    {
      traffic::TrafficInfo::RoadSegmentId roadSegment(
          seg.GetFeatureId(), seg.GetSegmentIdx(),
          seg.IsForward() ? traffic::TrafficInfo::RoadSegmentId::kForwardDirection
                          : traffic::TrafficInfo::RoadSegmentId::kReverseDirection);

      auto segTraffic = SpeedGroup::Unknown;
      if (auto trafficColoring = trafficStash->Get(seg.GetMwmId()))
      {
        auto const it = trafficColoring->find(roadSegment);
        if (it != trafficColoring->cend())
          segTraffic = it->second;
      }
      // @TODO It's written to compensate an error. The problem is in case of any routing except for osrm
      // all route points except for begining and ending are duplicated.
      // See a TODO in BicycleDirectionsEngine::Generate() for details.
      // At this moment the route looks like:
      // 0----1    4----5
      //      2----3    6---7
      // This route consists of 4 edges and there're 4 items in trafficSegs.
      // But it has 8 items in routeGeometry.
      // So for segments 0-1 and 1-2 let's set trafficSegs[0]
      // for segments 2-3 and 3-4 let's set trafficSegs[1]
      // for segments 4-5 and 5-6 let's set trafficSegs[2]
      // and for segment 6-7 let's set trafficSegs[3]
      traffic.insert(traffic.end(), {segTraffic, segTraffic});
    }
    CHECK(!traffic.empty(), ());
    traffic.pop_back();
  }

  route.SetTraffic(move(traffic));
}
}  // namespace rouing
