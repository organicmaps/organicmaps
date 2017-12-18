#include "openlr/candidate_paths_getter.hpp"

#include "openlr/candidate_points_getter.hpp"
#include "openlr/graph.hpp"
#include "openlr/helpers.hpp"
#include "openlr/openlr_model.hpp"

#include "routing/road_graph.hpp"

#include "platform/location.hpp"

#include "geometry/angles.hpp"

#include <algorithm>
#include <iterator>
#include <queue>
#include <set>
#include <tuple>

using namespace std;
using namespace routing;

namespace openlr
{
namespace
{
// TODO(mgsergio): Maybe add a penalty if this value deviates, not just throw it away.
int const kFRCThreshold = 3;

int const kNumBuckets = 256;
double const kAnglesInBucket = 360.0 / kNumBuckets;

m2::PointD PointAtSegmentM(m2::PointD const & p1, m2::PointD const & p2, double const distanceM)
{
  auto const v = p2 - p1;
  auto const l = v.Length();
  auto const L = MercatorBounds::DistanceOnEarth(p1, p2);
  auto const delta = distanceM * l / L;
  return PointAtSegment(p1, p2, delta);
}

uint32_t Bearing(m2::PointD const & a, m2::PointD const & b)
{
  auto const angle = location::AngleToBearing(my::RadToDeg(ang::AngleTo(a, b)));
  CHECK_LESS_OR_EQUAL(angle, 360, ("Angle should be less than or equal to 360."));
  CHECK_GREATER_OR_EQUAL(angle, 0, ("Angle should be greater than or equal to 0"));
  return my::clamp(angle / kAnglesInBucket, 0.0, 255.0);
}

// This class is used to get correct points for further bearing calculations.
// Depending on |isLastPoint| it either calculates those points straightforwardly
// or reverses directions and then calculates.
class BearingPointsSelector
{
public:
  BearingPointsSelector(uint32_t const bearDistM, bool const isLastPoint)
    : m_bearDistM(bearDistM), m_isLastPoint(isLastPoint)
  {
  }

  m2::PointD GetBearingStartPoint(Graph::Edge const & e) const
  {
    return m_isLastPoint ? e.GetEndPoint() : e.GetStartPoint();
  }

  m2::PointD GetBearingEndPoint(Graph::Edge const & e, uint32_t const distanceM)
  {
    if (distanceM < m_bearDistM && m_bearDistM <= distanceM + EdgeLength(e))
    {
      auto const edgeLen = EdgeLength(e);
      auto const edgeBearDist = min(m_bearDistM - distanceM, edgeLen);
      ASSERT_LESS_OR_EQUAL(edgeBearDist, edgeLen, ());
      return m_isLastPoint ? PointAtSegmentM(e.GetEndPoint(), e.GetStartPoint(),
                                             static_cast<double>(edgeBearDist))
                           : PointAtSegmentM(e.GetStartPoint(), e.GetEndPoint(),
                                             static_cast<double>(edgeBearDist));
    }
    return m_isLastPoint ? e.GetStartPoint() : e.GetEndPoint();
  }

private:
  uint32_t m_bearDistM;
  bool m_isLastPoint;
};
}  // namespace

// CandidatePathsGetter::Link ----------------------------------------------------------------------
bool CandidatePathsGetter::Link::operator<(Link const & o) const
{
  if (m_distanceM != o.m_distanceM)
    return m_distanceM < o.m_distanceM;

  if (m_edge != o.m_edge)
    return m_edge < o.m_edge;

  if (m_parent == o.m_parent)
    return false;

  if (m_parent && o.m_parent)
    return *m_parent < *o.m_parent;

  if (!m_parent)
    return true;

  return false;
}

Graph::Edge CandidatePathsGetter::Link::GetStartEdge() const
{
  auto * start = this;
  while (start->m_parent)
    start = start->m_parent.get();

  return start->m_edge;
}

bool CandidatePathsGetter::Link::IsJunctionInPath(routing::Junction const & j) const
{
  for (auto * l = this; l; l = l->m_parent.get())
  {
    if (l->m_edge.GetEndJunction() == j)
    {
      LOG(LDEBUG, ("A loop detected, skipping..."));
      return true;
    }
  }

  return false;
}

// CandidatePathsGetter ----------------------------------------------------------------------------
bool CandidatePathsGetter::GetLineCandidatesForPoints(
    vector<LocationReferencePoint> const & points,
    vector<vector<Graph::EdgeVector>> & lineCandidates)
{
  for (size_t i = 0; i < points.size(); ++i)
  {
    if (i != points.size() - 1 && points[i].m_distanceToNextPoint == 0)
    {
      LOG(LINFO, ("Distance to next point is zero. Skipping the whole segment"));
      ++m_stats.m_dnpIsZero;
      return false;
    }

    lineCandidates.emplace_back();
    auto const isLastPoint = i == points.size() - 1;
    auto const distanceToNextPoint =
        (isLastPoint ? points[i - 1] : points[i]).m_distanceToNextPoint;

    vector<m2::PointD> pointCandidates;
    m_pointsGetter.GetCandidatePoints(MercatorBounds::FromLatLon(points[i].m_latLon),
                                      pointCandidates);
    GetLineCandidates(points[i], isLastPoint, distanceToNextPoint, pointCandidates,
                      lineCandidates.back());

    if (lineCandidates.back().empty())
    {
      LOG(LINFO, ("No candidate lines found for point", points[i].m_latLon, "Giving up"));
      ++m_stats.m_noCandidateFound;
      return false;
    }
  }

  ASSERT_EQUAL(lineCandidates.size(), points.size(), ());

  return true;
}

void CandidatePathsGetter::GetStartLines(vector<m2::PointD> const & points, bool const isLastPoint,
                                         Graph::EdgeVector & edges)
{
  for (auto const & pc : points)
  {
    if (!isLastPoint)
      m_graph.GetOutgoingEdges(Junction(pc, 0 /* altitude */), edges);
    else
      m_graph.GetIngoingEdges(Junction(pc, 0 /* altitude */), edges);
  }

  // Same edges may start on different points if those points are close enough.
  my::SortUnique(edges, less<Graph::Edge>(), EdgesAreAlmostEqual);
}

void CandidatePathsGetter::GetAllSuitablePaths(Graph::EdgeVector const & startLines,
                                               bool const isLastPoint, uint32_t const bearDistM,
                                               FunctionalRoadClass const frc,
                                               vector<LinkPtr> & allPaths)
{
  queue<LinkPtr> q;

  for (auto const & e : startLines)
  {
    auto const u = make_shared<Link>(nullptr /* parent */, e, 0 /* distanceM */);
    q.push(u);
  }

  while (!q.empty())
  {
    auto const u = q.front();
    q.pop();

    auto const & currentEdge = u->m_edge;
    auto const currentEdgeLen = EdgeLength(currentEdge);

    // TODO(mgsergio): Maybe weak this constraint a bit.
    if (u->m_distanceM + currentEdgeLen >= bearDistM)
    {
      allPaths.push_back(u);
      continue;
    }

    ASSERT_LESS(u->m_distanceM + currentEdgeLen, bearDistM, ());

    Graph::EdgeVector edges;
    if (!isLastPoint)
      m_graph.GetOutgoingEdges(currentEdge.GetEndJunction(), edges);
    else
      m_graph.GetIngoingEdges(currentEdge.GetStartJunction(), edges);

    for (auto const & e : edges)
    {
      if (EdgesAreAlmostEqual(e.GetReverseEdge(), currentEdge))
        continue;

      ASSERT(currentEdge.HasRealPart(), ());

      if (!PassesRestriction(e, frc, kFRCThreshold, m_infoGetter))
        continue;

      // TODO(mgsergio): Should we check form of way as well?

      if (u->IsJunctionInPath(e.GetEndJunction()))
        continue;

      auto const p = make_shared<Link>(u, e, u->m_distanceM + currentEdgeLen);
      q.push(p);
    }
  }
}

void CandidatePathsGetter::GetBestCandidatePaths(
    vector<LinkPtr> const & allPaths, bool const isLastPoint, uint32_t const requiredBearing,
    uint32_t const bearDistM, m2::PointD const & startPoint, vector<Graph::EdgeVector> & candidates)
{
  set<CandidatePath> candidatePaths;
  set<CandidatePath> fakeEndingsCandidatePaths;

  BearingPointsSelector pointsSelector(bearDistM, isLastPoint);
  for (auto const & l : allPaths)
  {
    auto const bearStartPoint = pointsSelector.GetBearingStartPoint(l->GetStartEdge());
    auto const startPointsDistance =
        static_cast<uint32_t>(MercatorBounds::DistanceOnEarth(bearStartPoint, startPoint));

    // Number of edges counting from the last one to check bearing on. Accorfing to OpenLR spec
    // we have to check bearing on a point that is no longer than 25 meters traveling down the path.
    // But sometimes we may skip the best place to stop and generate a candidate. So we check several
    // edges before the last one to avoid such a situation. Number of iterations is taken
    // by intuition.
    // Example:
    // o -------- o  { Partners segment. }
    // o ------- o --- o { Our candidate. }
    //               ^ 25m
    //           ^ This one may be better than
    //                 ^ this one.
    // So we want to check them all.
    uint32_t traceBackIterationsLeft = 3;
    for (auto part = l; part; part = part->m_parent)
    {
      if (traceBackIterationsLeft == 0)
        break;

      --traceBackIterationsLeft;

      auto const bearEndPoint =
          pointsSelector.GetBearingEndPoint(part->m_edge, part->m_distanceM);

      auto const bearing = Bearing(bearStartPoint, bearEndPoint);
      auto const bearingDiff = AbsDifference(bearing, requiredBearing);
      auto const pathDistDiff = AbsDifference(part->m_distanceM, bearDistM);

      // TODO(mgsergio): Check bearing is within tolerance. Add to canidates if it is.

      if (part->m_hasFake)
        fakeEndingsCandidatePaths.emplace(part, bearingDiff, pathDistDiff, startPointsDistance);
      else
        candidatePaths.emplace(part, bearingDiff, pathDistDiff, startPointsDistance);
    }
  }

  ASSERT(
      none_of(begin(candidatePaths), end(candidatePaths), mem_fn(&CandidatePath::HasFakeEndings)),
      ());
  ASSERT(fakeEndingsCandidatePaths.empty() ||
             any_of(begin(fakeEndingsCandidatePaths), end(fakeEndingsCandidatePaths),
                    mem_fn(&CandidatePath::HasFakeEndings)),
         ());

  vector<CandidatePath> paths;
  copy_n(begin(candidatePaths), min(static_cast<size_t>(kMaxCandidates), candidatePaths.size()),
         back_inserter(paths));

  copy_n(begin(fakeEndingsCandidatePaths),
         min(static_cast<size_t>(kMaxFakeCandidates), fakeEndingsCandidatePaths.size()),
         back_inserter(paths));

  LOG(LDEBUG, ("List candidate paths..."));
  for (auto const & path : paths)
  {
    LOG(LDEBUG, ("CP:", path.m_bearingDiff, path.m_pathDistanceDiff, path.m_startPointDistance));
    Graph::EdgeVector edges;
    for (auto * p = path.m_path.get(); p; p = p->m_parent.get())
      edges.push_back(p->m_edge);
    if (!isLastPoint)
      reverse(begin(edges), end(edges));

    candidates.emplace_back(move(edges));
  }
}

void CandidatePathsGetter::GetLineCandidates(openlr::LocationReferencePoint const & p,
                                             bool const isLastPoint,
                                             uint32_t const distanceToNextPoint,
                                             vector<m2::PointD> const & pointCandidates,
                                             vector<Graph::EdgeVector> & candidates)
{
  uint32_t const kDefaultBearDistM = 25;
  uint32_t const bearDistM = min(kDefaultBearDistM, distanceToNextPoint);

  LOG(LINFO, ("BearDist is", bearDistM));

  Graph::EdgeVector startLines;
  GetStartLines(pointCandidates, isLastPoint, startLines);

  LOG(LINFO, (startLines.size(), "start line candidates found for point (LatLon)", p.m_latLon));
  LOG(LDEBUG, ("Listing start lines:"));
  for (auto const & e : startLines)
    LOG(LDEBUG, (LogAs2GisPath(e)));

  auto const startPoint = MercatorBounds::FromLatLon(p.m_latLon);

  vector<LinkPtr> allPaths;
  GetAllSuitablePaths(startLines, isLastPoint, bearDistM, p.m_functionalRoadClass, allPaths);
  GetBestCandidatePaths(allPaths, isLastPoint, p.m_bearing, bearDistM, startPoint, candidates);
  LOG(LDEBUG, (candidates.size(), "candidate paths found for point (LatLon)", p.m_latLon));
}
}  // namespace openlr
