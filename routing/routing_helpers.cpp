#include "routing/routing_helpers.hpp"
#include "routing/road_point.hpp"

#include "traffic/traffic_info.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <vector>

namespace
{
double constexpr KMPH2MPS = 1000.0 / (60 * 60);
}  // namespace

namespace routing
{
using namespace std;
using namespace traffic;

void ReconstructRoute(IDirectionsEngine & engine, RoadGraphBase const & graph,
                      shared_ptr<TrafficStash> const & trafficStash,
                      my::Cancellable const & cancellable, bool hasAltitude,
                      vector<Junction> const & path, Route::TTimes && times, Route & route)
{
  if (path.empty())
  {
    LOG(LERROR, ("Can't reconstruct route from an empty list of positions."));
    return;
  }

  CHECK_EQUAL(path.size(), times.size(), ());

  Route::TTimes dummyTimes;
  Route::TTurns turnsDir;
  vector<Junction> junctions;
  Route::TStreets streetNames;
  vector<Segment> segments;
  // @TODO It's necessary to remove |dummyTimes| from Generate() and MakeTurnAnnotation() methods..
  engine.Generate(graph, path, cancellable, dummyTimes, turnsDir, streetNames, junctions, segments);
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
      traffic.push_back(trafficStash->GetSpeedGroup(seg));
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

void CalculateMaxSpeedTimes(RoadGraphBase const & graph, vector<Junction> const & path,
                            Route::TTimes & times)
{
  times.clear();
  if (path.size() < 1)
    return;

  // graph.GetMaxSpeedKMPH() below is used on purpose.
  // The idea is while pedestrian (bicycle) routing ways for pedestrians (cyclists) are preferred.
  // At the same time routing along big roads is still possible but if there's
  // a pedestrian (bicycle) alternative it's prefered. To implement it a small speed
  // is set in pedestrian_model (bicycle_model) for big roads. On the other hand
  // the most likely a pedestrian (a cyclist) will go along big roads with average
  // speed (graph.GetMaxSpeedKMPH()).
  double const speedMPS = graph.GetMaxSpeedKMPH() * KMPH2MPS;

  times.reserve(path.size());

  double trackTimeSec = 0.0;
  times.emplace_back(0, trackTimeSec);

  m2::PointD prev = path[0].GetPoint();
  for (size_t i = 1; i < path.size(); ++i)
  {
    m2::PointD const & curr = path[i].GetPoint();

    double const lengthM = MercatorBounds::DistanceOnEarth(prev, curr);
    trackTimeSec += lengthM / speedMPS;

    times.emplace_back(i, trackTimeSec);

    prev = curr;
  }
}
}  // namespace routing
