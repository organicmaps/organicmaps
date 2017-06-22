#include "routing/routing_helpers.hpp"
#include "routing/road_point.hpp"

#include "traffic/traffic_info.hpp"

namespace routing
{
using namespace traffic;

void ReconstructRoute(IDirectionsEngine & engine, RoadGraphBase const & graph,
                      shared_ptr<TrafficStash> const & trafficStash,
                      my::Cancellable const & cancellable, bool hasAltitude,
                      vector<Junction> & path, Route & route)
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
  vector<Segment> segments;
  engine.Generate(graph, path, cancellable, times, turnsDir, junctions, segments);

  if (cancellable.IsCancelled())
    return;

  // In case of any errors in IDirectionsEngine::Generate() |junctions| is empty.
  if (junctions.empty())
  {
    LOG(LERROR, ("Internal error happened while turn generation."));
    return;
  }

  // @TODO(bykoianko) If the start and the finish of a route lies on the same road segment
  // engine->Generate() fills with empty vectors |times|, |turnsDir|, |junctions| and |segments|.
  // It's not correct and should be fixed. It's necessary to work corrrectly with such routes.

  vector<m2::PointD> routeGeometry;
  JunctionsToPoints(junctions, routeGeometry);

  route.SetGeometry(routeGeometry.begin(), routeGeometry.end());
  route.SetSectionTimes(move(times));
  route.SetTurnInstructions(move(turnsDir));
  route.SetStreetNames(move(streetNames));
  if (hasAltitude)
  {
    feature::TAltitudes altitudes;
    JunctionsToAltitudes(junctions, altitudes);
    route.SetAltitudes(move(altitudes));
  }

  vector<traffic::SpeedGroup> traffic;
  if (trafficStash && !segments.empty())
  {
    traffic.reserve(segments.size());
    for (Segment const & seg : segments)
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
      traffic.push_back(segTraffic);
    }
    CHECK_EQUAL(segments.size(), traffic.size(), ());
  }

  route.SetTraffic(move(traffic));
}
}  // namespace rouing
