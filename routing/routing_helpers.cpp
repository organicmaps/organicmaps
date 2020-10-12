#include "routing/routing_helpers.hpp"

#include "routing/road_point.hpp"
#include "routing/segment.hpp"

#include "traffic/traffic_info.hpp"

#include "geometry/point2d.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <utility>

namespace routing
{
using namespace std;
using namespace traffic;

void FillSegmentInfo(vector<Segment> const & segments,
                     vector<geometry::PointWithAltitude> const & junctions,
                     Route::TTurns const & turns, Route::TStreets const & streets,
                     Route::TTimes const & times, shared_ptr<TrafficStash> const & trafficStash,
                     vector<RouteSegment> & routeSegment)
{
  CHECK_EQUAL(segments.size() + 1, junctions.size(), ());
  CHECK(!turns.empty(), ());
  CHECK(!times.empty(), ());

  CHECK(is_sorted(turns.cbegin(), turns.cend(), base::LessBy(&turns::TurnItem::m_index)), ());
  CHECK(is_sorted(streets.cbegin(), streets.cend(), base::LessBy(&Route::TStreetItem::first)), ());
  CHECK(is_sorted(times.cbegin(), times.cend(), base::LessBy(&Route::TTimeItem::first)), ());

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
      mercator::DistanceOnEarth(junctions[i].GetPoint(), junctions[i + 1].GetPoint());
    routeLengthMerc += junctions[i].GetPoint().Length(junctions[i + 1].GetPoint());

    routeSegment.emplace_back(
        segments[i], curTurn, junctions[i + 1], curStreet, routeLengthMeters, routeLengthMerc,
        timeFromBeginningS,
        trafficStash ? trafficStash->GetSpeedGroup(segments[i]) : traffic::SpeedGroup::Unknown,
        nullptr /* transitInfo */);
  }
}

void ReconstructRoute(DirectionsEngine & engine, IndexRoadGraph const & graph,
                      shared_ptr<TrafficStash> const & trafficStash,
                      base::Cancellable const & cancellable,
                      vector<geometry::PointWithAltitude> const & path, Route::TTimes && times,
                      Route & route)
{
  if (path.empty())
  {
    LOG(LERROR, ("Can't reconstruct route from an empty list of positions."));
    return;
  }

  CHECK_EQUAL(path.size(), times.size(), ());

  Route::TTurns turnsDir;
  vector<geometry::PointWithAltitude> junctions;
  Route::TStreets streetNames;
  vector<Segment> segments;

  if (!engine.Generate(graph, path, cancellable, turnsDir, streetNames, junctions, segments))
    return;

  if (cancellable.IsCancelled())
    return;

  // In case of any errors in DirectionsEngine::Generate() |junctions| is empty.
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
  {
    if (edge.HasRealPart())
    {
      return Segment(kFakeNumMwmId, FakeFeatureIds::kIndexGraphStarterId, edge.GetFakeSegmentId(),
                     true /* forward */);
    }

    return Segment();
  }

  NumMwmId const numMwmId =
      numMwmIds.GetId(edge.GetFeatureId().m_mwmId.GetInfo()->GetLocalFile().GetCountryFile());

  return Segment(numMwmId, edge.GetFeatureId().m_index, edge.GetSegId(), edge.IsForward());
}

bool SegmentCrossesRect(m2::Segment2D const & segment, m2::RectD const & rect)
{
  double constexpr kEps = 1e-6;
  bool isSideIntersected = false;
  rect.ForEachSide([&segment, &isSideIntersected](m2::PointD const & a, m2::PointD const & b) {
    if (isSideIntersected)
      return;

    m2::Segment2D const rectSide(a, b);
    isSideIntersected =
        m2::Intersect(segment, rectSide, kEps).m_type != m2::IntersectionResult::Type::Zero;
  });

  return isSideIntersected;
}

bool RectCoversPolyline(IRoadGraph::PointWithAltitudeVec const & junctions, m2::RectD const & rect)
{
  if (junctions.empty())
    return false;

  if (junctions.size() == 1)
    return rect.IsPointInside(junctions.front().GetPoint());

  for (auto const & junction : junctions)
  {
    if (rect.IsPointInside(junction.GetPoint()))
      return true;
  }

  // No point of polyline |junctions| lays inside |rect| but may be segments of the polyline
  // cross |rect| borders.
  for (size_t i = 1; i < junctions.size(); ++i)
  {
    m2::Segment2D const polylineSegment(junctions[i - 1].GetPoint(), junctions[i].GetPoint());
    if (SegmentCrossesRect(polylineSegment, rect))
      return true;
  }

  return false;
}

bool CheckGraphConnectivity(Segment const & start, bool isOutgoing, bool useRoutingOptions,
                            size_t limit, WorldGraph & graph, std::set<Segment> & marked)
{
  std::queue<Segment> q;
  q.push(start);

  marked.insert(start);

  std::vector<SegmentEdge> edges;
  while (!q.empty() && marked.size() < limit)
  {
    auto const u = q.front();
    q.pop();

    edges.clear();

    // Note. If |isOutgoing| == true outgoing edges are looked for.
    // If |isOutgoing| == false it's the finish. So ingoing edges are looked for.
    graph.GetEdgeList(u, isOutgoing, useRoutingOptions, edges);
    for (auto const & edge : edges)
    {
      auto const & v = edge.GetTarget();
      if (marked.count(v) == 0)
      {
        q.push(v);
        marked.insert(v);
      }
    }
  }

  return marked.size() >= limit;
}

// AStarLengthChecker ------------------------------------------------------------------------------

AStarLengthChecker::AStarLengthChecker(IndexGraphStarter & starter) : m_starter(starter) {}

bool AStarLengthChecker::operator()(RouteWeight const & weight) const
{
  return m_starter.CheckLength(weight);
}

// AdjustLengthChecker -----------------------------------------------------------------------------

AdjustLengthChecker::AdjustLengthChecker(IndexGraphStarter & starter) : m_starter(starter) {}

bool AdjustLengthChecker::operator()(RouteWeight const & weight) const
{
  // Limit of adjust in seconds.
  double constexpr kAdjustLimitSec = 5 * 60;
  return weight <= RouteWeight(kAdjustLimitSec) && m_starter.CheckLength(weight);
}
}  // namespace routing
