#include "routing/routing_helpers.hpp"
#include "routing/road_point.hpp"
#include "routing/segment.hpp"

#include "traffic/traffic_info.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <utility>

namespace
{
double constexpr KMPH2MPS = 1000.0 / (60 * 60);
}  // namespace

namespace routing
{
using namespace std;
using namespace traffic;

void FillSegmentInfo(vector<Segment> const & segments, vector<Junction> const & junctions,
                     Route::TTurns const & turns, Route::TStreets const & streets,
                     Route::TTimes const & times, shared_ptr<TrafficStash> const & trafficStash,
                     vector<RouteSegment> & routeSegment)
{
  CHECK_EQUAL(segments.size() + 1, junctions.size(), ());
  CHECK(!turns.empty(), ());
  CHECK(!times.empty(), ());

  CHECK(is_sorted(turns.cbegin(), turns.cend(), my::LessBy(&turns::TurnItem::m_index)), ());
  CHECK(is_sorted(streets.cbegin(), streets.cend(), my::LessBy(&Route::TStreetItem::first)), ());
  CHECK(is_sorted(times.cbegin(), times.cend(), my::LessBy(&Route::TTimeItem::first)), ());

  CHECK_LESS(turns.back().m_index, junctions.size(), ());
  if (!streets.empty())
    CHECK_LESS(streets.back().first, junctions.size(), ());
  CHECK_LESS(times.back().first, junctions.size(), ());

  routeSegment.clear();
  if (segments.empty())
    return;

  routeSegment.reserve(segments.size());
  // Note. |turns|, |streets| and |times| have point based index.
  // On the other hand turns, street names and times are considered for further end of |segments| after conversion
  // to |segmentInfo|. It means that street, turn and time information of zero point will be lost after
  // conversion to |segmentInfo|.
  size_t turnIdx = turns[0].m_index != 0 ? 0 : 1;
  size_t streetIdx = (!streets.empty() && streets[0].first != 0) ? 0 : 1;
  size_t timeIdx = times[0].first != 0 ? 0 : 1;
  double routeLengthMeters = 0.0;
  double routeLengthMerc = 0.0;
  double timeFromBeginningS = 0.0;

  for (size_t i = 0; i < segments.size(); ++i)
  {
    size_t const segmentEndPointIdx = i + 1;

    turns::TurnItem curTurn;
    CHECK_LESS_OR_EQUAL(turnIdx, turns.size(), ());
    if (turnIdx != turns.size() && turns[turnIdx].m_index == segmentEndPointIdx)
    {
      curTurn = turns[turnIdx];
      if (turnIdx + 1 < turns.size())
        ++turnIdx;
    }

    string curStreet;
    if (!streets.empty())
    {
      CHECK_LESS_OR_EQUAL(streetIdx, streets.size(), ());
      if (streetIdx != streets.size() && streets[streetIdx].first == segmentEndPointIdx)
      {
        curStreet = streets[streetIdx].second;
        if (streetIdx + 1 < streets.size())
          ++streetIdx;
      }
    }

    CHECK_LESS_OR_EQUAL(timeIdx, times.size(), ());
    if (timeIdx != times.size() && times[timeIdx].first == segmentEndPointIdx)
    {
      timeFromBeginningS = times[timeIdx].second;
      if (timeIdx + 1 < times.size())
        ++timeIdx;
    }

    routeLengthMeters +=
      MercatorBounds::DistanceOnEarth(junctions[i].GetPoint(), junctions[i + 1].GetPoint());
    routeLengthMerc += junctions[i].GetPoint().Length(junctions[i + 1].GetPoint());

    routeSegment.emplace_back(
        segments[i], curTurn, junctions[i + 1], curStreet, routeLengthMeters, routeLengthMerc,
        timeFromBeginningS,
        trafficStash ? trafficStash->GetSpeedGroup(segments[i]) : traffic::SpeedGroup::Unknown,
        nullptr /* transitInfo */);
  }
}

void ReconstructRoute(IDirectionsEngine & engine, IndexRoadGraph const & graph,
                      shared_ptr<TrafficStash> const & trafficStash,
                      my::Cancellable const & cancellable, vector<Junction> const & path,
                      Route::TTimes && times, Route & route)
{
  if (path.empty())
  {
    LOG(LERROR, ("Can't reconstruct route from an empty list of positions."));
    return;
  }

  CHECK_EQUAL(path.size(), times.size(), ());

  Route::TTurns turnsDir;
  vector<Junction> junctions;
  Route::TStreets streetNames;
  vector<Segment> segments;

  if (!engine.Generate(graph, path, cancellable, turnsDir, streetNames, junctions, segments))
    return;

  if (cancellable.IsCancelled())
    return;

  // In case of any errors in IDirectionsEngine::Generate() |junctions| is empty.
  if (junctions.empty())
  {
    LOG(LERROR, ("Internal error happened while turn generation."));
    return;
  }

  CHECK_EQUAL(path.size(), junctions.size(),
              ("Size of path:", path.size(), "size of junctions:", junctions.size()));

  vector<RouteSegment> segmentInfo;
  FillSegmentInfo(segments, junctions, turnsDir, streetNames, times, trafficStash, segmentInfo);
  CHECK_EQUAL(segmentInfo.size(), segments.size(), ());
  route.SetRouteSegments(move(segmentInfo));

  // @TODO(bykoianko) If the start and the finish of a route lies on the same road segment
  // engine->Generate() fills with empty vectors |times|, |turnsDir|, |junctions| and |segments|.
  // It's not correct and should be fixed. It's necessary to work correctly with such routes.

  vector<m2::PointD> routeGeometry;
  JunctionsToPoints(junctions, routeGeometry);

  route.SetGeometry(routeGeometry.begin(), routeGeometry.end());
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
  if (path.empty())
    return;

  // graph.GetMaxSpeedKMPH() below is used on purpose.
  // The idea is while pedestrian (bicycle) routing ways for pedestrians (cyclists) are preferred.
  // At the same time routing along big roads is still possible but if there's
  // a pedestrian (bicycle) alternative it's prefered. To implement it a small speed
  // is set in pedestrian_model (bicycle_model) for big roads. On the other hand
  // the most likely a pedestrian (a cyclist) will go along big roads with average
  // speed (graph.GetMaxSpeedKMPH()).
  double const speedMPS = graph.GetMaxSpeedKMPH() * KMPH2MPS;
  CHECK_GREATER(speedMPS, 0.0, ());

  times.reserve(path.size());

  double trackTimeSec = 0.0;
  times.emplace_back(0, trackTimeSec);

  for (size_t i = 1; i < path.size(); ++i)
  {
    double const lengthM =
        MercatorBounds::DistanceOnEarth(path[i - 1].GetPoint(), path[i].GetPoint());
    trackTimeSec += lengthM / speedMPS;

    times.emplace_back(i, trackTimeSec);
  }
  CHECK_EQUAL(times.size(), path.size(), ());
}
}  // namespace routing
