#include "routing/routing_helpers.hpp"
#include "routing/road_point.hpp"

#include "traffic/traffic_info.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>

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
  Route::TStreets streetNames;
  vector<Segment> segments;
  engine.Generate(graph, path, cancellable, times, turnsDir, streetNames, junctions, segments);
  CHECK_EQUAL(segments.size() + 1, junctions.size(), ());

  if (cancellable.IsCancelled())
    return;

  // In case of any errors in IDirectionsEngine::Generate() |junctions| is empty.
  if (junctions.empty())
  {
    LOG(LERROR, ("Internal error happened while turn generation."));
    return;
  }

  CHECK(std::is_sorted(times.cbegin(), times.cend(), my::LessBy(&Route::TTimeItem::first)), ());
  CHECK(std::is_sorted(turnsDir.cbegin(), turnsDir.cend(), my::LessBy(&turns::TurnItem::m_index)), ());
  CHECK(std::is_sorted(streetNames.cbegin(), streetNames.cend(), my::LessBy(&Route::TStreetItem::first)), ());

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

Segment ConvertEdgeToSegment(NumMwmIds const & numMwmIds, Edge const & edge)
{
  if (edge.IsFake())
    return Segment();


  NumMwmId const numMwmId =
      numMwmIds.GetId(edge.GetFeatureId().m_mwmId.GetInfo()->GetLocalFile().GetCountryFile());
  return Segment(numMwmId, edge.GetFeatureId().m_index, edge.GetSegId(), edge.IsForward());
}
}  // namespace rouing
