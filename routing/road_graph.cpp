#include "routing/road_graph.hpp"
#include "routing/road_graph_router.hpp"

#include "routing/route.hpp"

#include "indexer/mercator.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/assert.hpp"

#include "std/limits.hpp"
#include "std/sstream.hpp"

namespace routing
{

namespace
{
double constexpr KMPH2MPS = 1000.0 / (60 * 60);

inline double CalcDistanceMeters(m2::PointD const & p1, m2::PointD const & p2)
{
  return MercatorBounds::DistanceOnEarth(p1, p2);
}

inline double TimeBetweenSec(m2::PointD const & p1, m2::PointD const & p2, double speedMPS)
{
  ASSERT(speedMPS > 0.0, ());
  return CalcDistanceMeters(p1, p2) / speedMPS;
}

inline double TimeBetweenSec(Junction const & j1, Junction const & j2, double speedMPS)
{
  return TimeBetweenSec(j1.GetPoint(), j2.GetPoint(), speedMPS);
}

inline bool PointsAlmostEqual(const m2::PointD & pt1, const m2::PointD & pt2)
{
  double constexpr EPSILON = 1e-6;
  if ((pt2.x < (pt1.x - EPSILON)) || (pt2.x > (pt1.x + EPSILON)))
    return false;
  if ((pt2.y < (pt1.y - EPSILON)) || (pt2.y > (pt1.y + EPSILON)))
    return false;
  return true;
}

} // namespace

// Junction --------------------------------------------------------------------

Junction::Junction()
  : m_point(m2::PointD::Zero())
{}

Junction::Junction(m2::PointD const & point)
  : m_point(point)
{}

string DebugPrint(Junction const & r)
{
  ostringstream ss;
  ss << "Junction{point:" << DebugPrint(r.m_point) << "}";
  return ss.str();
}

// Edge ------------------------------------------------------------------------

Edge Edge::MakeFake(Junction const & startJunction, Junction const & endJunction)
{
  return Edge(kFakeFeatureId, true /* forward */, 0 /* segId */, startJunction, endJunction);
}

Edge::Edge(uint32_t featureId, bool forward, size_t segId, Junction const & startJunction, Junction const & endJunction)
  : m_featureId(featureId), m_forward(forward), m_segId(segId), m_startJunction(startJunction), m_endJunction(endJunction)
{
  ASSERT_LESS(segId, numeric_limits<uint32_t>::max(), ());
}

Edge Edge::GetReverseEdge() const
{
  return Edge(m_featureId, !m_forward, m_segId, m_endJunction, m_startJunction);
}

bool Edge::SameRoadSegmentAndDirection(Edge const & r) const
{
  return m_featureId == r.m_featureId &&
         m_forward == r.m_forward &&
         m_segId == r.m_segId;
}

bool Edge::operator==(Edge const & r) const
{
  return m_featureId == r.m_featureId &&
         m_forward == r.m_forward &&
         m_segId == r.m_segId &&
         m_startJunction == r.m_startJunction &&
         m_endJunction == r.m_endJunction;
}

bool Edge::operator<(Edge const & r) const
{
  if (m_featureId != r.m_featureId)
    return m_featureId < r.m_featureId;
  if (m_forward != r.m_forward)
    return (m_forward == false);
  if (m_segId != r.m_segId)
    return m_segId < r.m_segId;
  if (!(m_startJunction == r.m_startJunction))
    return m_startJunction < r.m_startJunction;
  if (!(m_endJunction == r.m_endJunction))
    return m_endJunction < r.m_endJunction;
  return false;
}

string DebugPrint(Edge const & r)
{
  ostringstream ss;
  ss << "Edge{featureId: " << r.GetFeatureId() << ", isForward:" << r.IsForward()
     << ", segId:" << r.m_segId << ", startJunction:" << DebugPrint(r.m_startJunction)
     << ", endJunction:" << DebugPrint(r.m_endJunction) << "}";
  return ss.str();
}

// IRoadGraph::RoadInfo --------------------------------------------------------

IRoadGraph::RoadInfo::RoadInfo()
  : m_speedKMPH(0.0), m_bidirectional(false)
{}

IRoadGraph::RoadInfo::RoadInfo(RoadInfo && ri)
    : m_points(move(ri.m_points)),
      m_speedKMPH(ri.m_speedKMPH),
      m_bidirectional(ri.m_bidirectional)
{}

IRoadGraph::RoadInfo::RoadInfo(bool bidirectional, double speedKMPH, initializer_list<m2::PointD> const & points)
    : m_points(points), m_speedKMPH(speedKMPH), m_bidirectional(bidirectional)
{}

// IRoadGraph::CrossEdgesLoader ------------------------------------------------

IRoadGraph::CrossEdgesLoader::CrossEdgesLoader(m2::PointD const & cross, TEdgeVector & outgoingEdges)
    : m_cross(cross), m_outgoingEdges(outgoingEdges)
{
}

void IRoadGraph::CrossEdgesLoader::operator()(uint32_t featureId, RoadInfo const & roadInfo)
{
  if (roadInfo.m_points.empty())
    return;

  size_t const numPoints = roadInfo.m_points.size();

  for (size_t i = 0; i < numPoints; ++i)
  {
    m2::PointD const & p = roadInfo.m_points[i];

    if (!PointsAlmostEqual(m_cross, p))
      continue;

    if (i > 0)
    {
      //               p
      // o------------>o

      Edge edge(featureId, false /* forward */, i - 1, p, roadInfo.m_points[i - 1]);
      m_outgoingEdges.push_back(edge);
    }

    if (i < numPoints - 1)
    {
      // p
      // o------------>o

      Edge edge(featureId, true /* forward */, i, p, roadInfo.m_points[i + 1]);
      m_outgoingEdges.push_back(edge);
    }
  }
}

// IRoadGraph ------------------------------------------------------------------

void IRoadGraph::ReconstructPath(TJunctionVector const & positions, Route & route)
{
  CHECK(!positions.empty(), ("Can't reconstruct path from an empty list of positions."));

  vector<m2::PointD> path;
  path.reserve(positions.size());

  double const speedMPS = GetMaxSpeedKMPH() * KMPH2MPS;

  double trackTimeSec = 0.0;
  m2::PointD prevPoint = positions[0].GetPoint();
  path.push_back(prevPoint);
  for (size_t i = 1; i < positions.size(); ++i)
  {
    m2::PointD const curPoint = positions[i].GetPoint();

    // By some reason there're two adjacent positions on a road with
    // the same end-points. This could happen, for example, when
    // direction on a road was changed.  But it doesn't matter since
    // this code reconstructs only geometry of a route.
    if (curPoint == prevPoint)
      continue;

    path.push_back(curPoint);

    double const lengthM = CalcDistanceMeters(prevPoint, curPoint);
    trackTimeSec += lengthM / speedMPS;
    prevPoint = curPoint;
  }

  if (path.size() == 1)
  {
    m2::PointD point = path.front();
    path.push_back(point);
  }

  ASSERT_GREATER_OR_EQUAL(path.size(), 2, ());

  /// @todo: investigate whether it's worth reconstructing detailed turns and times.

  Route::TimesT times;
  times.emplace_back(path.size() - 1, trackTimeSec);

  Route::TurnsT turnsDir;
  turnsDir.emplace_back(path.size() - 1, turns::TurnDirection::ReachedYourDestination);

  route.SetGeometry(path.begin(), path.end());
  route.SetTurnInstructions(turnsDir);
  route.SetSectionTimes(times);
}

void IRoadGraph::GetOutgoingEdges(Junction const & junction, TEdgeVector & edges)
{
  auto const itr = m_outgoingEdges.find(junction);
  if (itr != m_outgoingEdges.end())
  {
    edges.reserve(edges.size() + itr->second.size());
    edges.insert(edges.end(), itr->second.begin(), itr->second.end());
  }
  else
  {
    GetRegularOutgoingEdges(junction, edges);
  }
}

void IRoadGraph::GetIngoingEdges(Junction const & junction, TEdgeVector & edges)
{
  size_t const wasSize = edges.size();

  GetOutgoingEdges(junction, edges);

  size_t const size = edges.size();
  for (size_t i = wasSize; i < size; ++i)
    edges[i] = edges[i].GetReverseEdge();
}

void IRoadGraph::GetRegularOutgoingEdges(Junction const & junction, TEdgeVector & edges)
{
  m2::PointD const cross = junction.GetPoint();
  CrossEdgesLoader loader(cross, edges);
  ForEachFeatureClosestToCross(cross, loader);
}

void IRoadGraph::ResetFakes()
{
  m_outgoingEdges.clear();
}

void IRoadGraph::AddFakeEdges(Junction const & junction, vector<pair<Edge, m2::PointD>> const & vicinity)
{
  for (auto const & v : vicinity)
  {
    Edge const & closestEdge = v.first;
    Junction const p = v.second;

    if (p == closestEdge.GetStartJunction() || p == closestEdge.GetEndJunction())
    {
      // The point is mapped on the start junction of the edge or on the end junction of the edge:
      //        o M                         o M
      //        ^                           ^
      //        |                           |
      //        |                           |
      //  (P) A o--------------->o B  or  A o--------------->o B (P)  (the feature is A->B)
      // Here AB is a feature, M is a junction, which is projected to A (where P is projection),
      // P is the closest junction of the feature to the junction M.

      // Add outgoing edges for M.
      TEdgeVector & edgesM = m_outgoingEdges[junction];
      edgesM.push_back(Edge::MakeFake(junction, p));

      // Add outgoing edges for P.
      TEdgeVector & edgesP = m_outgoingEdges[p];
      GetRegularOutgoingEdges(p, edgesP);
      edgesP.push_back(Edge::MakeFake(p, junction));
    }
    else
    {
      // The point is mapped in the middle of the feature:
      //            o M
      //            ^
      //            |
      //            |
      // A o<-------x------->o B  (the feature is A->B)
      //            P
      // Here AB is a feature, M is a junction and P is a projection of M on AB,
      // Edge AB has been splitted to two fake edges AP and PB (similarly BA edge has been splitted to two
      // fake edges BP and PA). In the result graph edges AB and BA are redundant, therefore edges AB and BA are
      // replaced by fake edges AP + PB and BP + PA.

      Edge const pa(closestEdge.GetFeatureId(), false, closestEdge.GetSegId(), p, closestEdge.GetStartJunction());
      Edge const pb(closestEdge.GetFeatureId(), true, closestEdge.GetSegId(), p, closestEdge.GetEndJunction());
      Edge const pm = Edge::MakeFake(p, junction);

      // Add outgoing edges to point P.
      TEdgeVector & edgesP = m_outgoingEdges[p];
      edgesP.push_back(pa);
      edgesP.push_back(pb);
      edgesP.push_back(pm);

      // Add outgoing edges for point M.
      m_outgoingEdges[junction].push_back(pm.GetReverseEdge());

      // Add outgoing edges for point A. AB edge will be replaced by AP edge.
      TEdgeVector & edgesA = m_outgoingEdges[pa.GetEndJunction()];
      GetRegularOutgoingEdges(pa.GetEndJunction(), edgesA);
      Edge const ap = pa.GetReverseEdge();
      edgesA.erase(remove_if(edgesA.begin(), edgesA.end(), [&](Edge const & e) { return e.SameRoadSegmentAndDirection(ap); }), edgesA.end());
      edgesA.push_back(ap);

      // Add outgoing edges for point B. BA edge will be replaced by BP edge.
      TEdgeVector & edgesB = m_outgoingEdges[pb.GetEndJunction()];
      GetRegularOutgoingEdges(pb.GetEndJunction(), edgesB);
      Edge const bp = pb.GetReverseEdge();
      edgesB.erase(remove_if(edgesB.begin(), edgesB.end(), [&](Edge const & e) { return e.SameRoadSegmentAndDirection(bp); }), edgesB.end());
      edgesB.push_back(bp);
    }
  }

  /// @todo There's case here when Start and Finish points are projected to the same segment of the feature.
  /// Then, feature can be split into 3 pieces.
  //            o M            o N
  //            ^              ^
  //            | fe           |
  //            |              |
  // A o<-------x--------------x------------->o B
  // Here AB is feature, M and N are junction, which is projected to A or to B.

  // m_outgoingEdges may contain duplicates. Remove them.
  for (auto & m : m_outgoingEdges)
  {
    TEdgeVector & edges = m.second;
    sort(edges.begin(), edges.end());
    edges.erase(unique(edges.begin(), edges.end()), edges.end());
  }
}

// RoadGraph -------------------------------------------------------------------

RoadGraph::RoadGraph(IRoadGraph & roadGraph) : m_roadGraph(roadGraph) {}

double RoadGraph::GetSpeedMPS(Edge const & e) const
{
  double const speedKMPH = (e.IsFake() ? m_roadGraph.GetMaxSpeedKMPH() : m_roadGraph.GetSpeedKMPH(e.GetFeatureId()));
  ASSERT(speedKMPH <= m_roadGraph.GetMaxSpeedKMPH(), ());

  return speedKMPH * KMPH2MPS;
}

void RoadGraph::GetOutgoingEdgesListImpl(Junction const & v, vector<WeightedEdge> & adj) const
{
  IRoadGraph::TEdgeVector edges;
  m_roadGraph.GetOutgoingEdges(v, edges);

  adj.clear();
  adj.reserve(edges.size());

  for (auto const & e : edges)
  {
    double const speedMPS = GetSpeedMPS(e);
    if (speedMPS <= 0.0)
      continue;

    ASSERT_EQUAL(v, e.GetStartJunction(), ());

    adj.emplace_back(e.GetEndJunction(), TimeBetweenSec(e.GetStartJunction(), e.GetEndJunction(), speedMPS));
  }
}

void RoadGraph::GetIngoingEdgesListImpl(Junction const & v, vector<WeightedEdge> & adj) const
{
  IRoadGraph::TEdgeVector edges;
  m_roadGraph.GetIngoingEdges(v, edges);

  adj.clear();
  adj.reserve(edges.size());

  for (auto const & e : edges)
  {
    double const speedMPS = GetSpeedMPS(e);
    if (speedMPS <= 0.0)
      continue;

    ASSERT_EQUAL(v, e.GetEndJunction(), ());

    adj.emplace_back(e.GetStartJunction(), TimeBetweenSec(e.GetStartJunction(), e.GetEndJunction(), speedMPS));
  }
}

double RoadGraph::HeuristicCostEstimateImpl(Junction const & v, Junction const & w) const
{
  double const speedMPS = m_roadGraph.GetMaxSpeedKMPH() * KMPH2MPS;
  return TimeBetweenSec(v, w, speedMPS);
}

}  // namespace routing
