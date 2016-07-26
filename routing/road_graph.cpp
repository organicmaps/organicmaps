#include "routing/road_graph.hpp"
#include "routing/road_graph_router.hpp"

#include "routing/route.hpp"

#include "geometry/mercator.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include "std/limits.hpp"
#include "std/sstream.hpp"

namespace routing
{
namespace
{
vector<Edge>::const_iterator FindEdgeContainingPoint(vector<Edge> const & edges, m2::PointD const & pt)
{
  // A           P               B
  // o-----------x---------------o

  auto const liesOnEdgeFn = [&pt](Edge const & e)
  {
    // Point P lies on edge AB if:
    // - P corresponds to edge's start point A or edge's end point B
    // - angle between PA and PB is 180 degrees

    m2::PointD const v1 = e.GetStartJunction().GetPoint() - pt;
    m2::PointD const v2 = e.GetEndJunction().GetPoint() - pt;
    if (my::AlmostEqualAbs(v1, m2::PointD::Zero(), kPointsEqualEpsilon)
        || my::AlmostEqualAbs(v2, m2::PointD::Zero(), kPointsEqualEpsilon))
    {
      // Point p corresponds to the start or the end of the edge.
      return true;
    }

    // If an angle between two vectors is 180 degrees then dot product is negative and cross product is 0.
    if (m2::DotProduct(v1, v2) < 0.0)
    {
        double constexpr kEpsilon = 1e-9;
        if (my::AlmostEqualAbs(m2::CrossProduct(v1, v2), 0.0, kEpsilon))
        {
            // Point p lies on edge.
            return true;
        }
    }

    return false;
  };

  return find_if(edges.begin(), edges.end(), liesOnEdgeFn);
}

/// \brief Reverses |edges| starting from index |beginIdx| and upto the end of |v|.
void ReverseEdges(size_t beginIdx, IRoadGraph::TEdgeVector & edges)
{
  ASSERT_LESS_OR_EQUAL(beginIdx, edges.size(), ("Index too large."));

  for (size_t i = beginIdx; i < edges.size(); ++i)
    edges[i] = edges[i].GetReverseEdge();
}
}  // namespace

// Junction --------------------------------------------------------------------

Junction::Junction() : m_point(m2::PointD::Zero()), m_altitude(feature::kInvalidAltitude) {}
Junction::Junction(m2::PointD const & point, feature::TAltitude altitude)
  : m_point(point), m_altitude(altitude)
{}

string DebugPrint(Junction const & r)
{
  ostringstream ss;
  ss << "Junction{point:" << DebugPrint(r.m_point) << ", altitude:}" << r.GetAltitude();
  return ss.str();
}

// Edge ------------------------------------------------------------------------

Edge Edge::MakeFake(Junction const & startJunction, Junction const & endJunction)
{
  return Edge(FeatureID(), true /* forward */, 0 /* segId */, startJunction, endJunction);
}

Edge::Edge(FeatureID const & featureId, bool forward, uint32_t segId, Junction const & startJunction, Junction const & endJunction)
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
  ss << "Edge{featureId: " << DebugPrint(r.GetFeatureId()) << ", isForward:" << r.IsForward()
     << ", segId:" << r.m_segId << ", startJunction:" << DebugPrint(r.m_startJunction)
     << ", endJunction:" << DebugPrint(r.m_endJunction) << "}";
  return ss.str();
}

// IRoadGraph::RoadInfo --------------------------------------------------------

IRoadGraph::RoadInfo::RoadInfo()
  : m_speedKMPH(0.0), m_bidirectional(false)
{}
IRoadGraph::RoadInfo::RoadInfo(RoadInfo && ri)
  : m_junctions(move(ri.m_junctions))
  , m_speedKMPH(ri.m_speedKMPH)
  , m_bidirectional(ri.m_bidirectional)
{}

IRoadGraph::RoadInfo::RoadInfo(bool bidirectional, double speedKMPH,
                               initializer_list<Junction> const & points)
  : m_junctions(points), m_speedKMPH(speedKMPH), m_bidirectional(bidirectional)
{}

// IRoadGraph::CrossOutgoingLoader ---------------------------------------------
void IRoadGraph::CrossOutgoingLoader::LoadEdges(FeatureID const & featureId, RoadInfo const & roadInfo)
{
  ForEachEdge(roadInfo, [&featureId, &roadInfo, this](size_t segId, Junction const & endJunction,
                                                      bool forward) {
    if (forward || roadInfo.m_bidirectional || m_mode == IRoadGraph::Mode::IgnoreOnewayTag)
      m_edges.emplace_back(featureId, forward, segId, m_cross, endJunction);
  });
}

// IRoadGraph::CrossIngoingLoader ----------------------------------------------
void IRoadGraph::CrossIngoingLoader::LoadEdges(FeatureID const & featureId, RoadInfo const & roadInfo)
{
  ForEachEdge(roadInfo, [&featureId, &roadInfo, this](size_t segId, Junction const & endJunction,
                                                      bool forward) {
    if (!forward || roadInfo.m_bidirectional || m_mode == IRoadGraph::Mode::IgnoreOnewayTag)
      m_edges.emplace_back(featureId, !forward, segId, endJunction, m_cross);
  });
}

// IRoadGraph ------------------------------------------------------------------
void IRoadGraph::GetOutgoingEdges(Junction const & junction, TEdgeVector & edges) const
{
  GetFakeOutgoingEdges(junction, edges);
  GetRegularOutgoingEdges(junction, edges);
}

void IRoadGraph::GetIngoingEdges(Junction const & junction, TEdgeVector & edges) const
{
  GetFakeIngoingEdges(junction, edges);
  GetRegularIngoingEdges(junction, edges);
}

void IRoadGraph::GetRegularOutgoingEdges(Junction const & junction, TEdgeVector & edges) const
{
  CrossOutgoingLoader loader(junction, GetMode(), edges);
  ForEachFeatureClosestToCross(junction.GetPoint(), loader);
}

void IRoadGraph::GetRegularIngoingEdges(Junction const & junction, TEdgeVector & edges) const
{
  CrossIngoingLoader loader(junction, GetMode(), edges);
  ForEachFeatureClosestToCross(junction.GetPoint(), loader);
}

void IRoadGraph::GetFakeOutgoingEdges(Junction const & junction, TEdgeVector & edges) const
{
  auto const itr = m_outgoingEdges.find(junction);
  if (itr == m_outgoingEdges.cend())
    return;

  edges.reserve(edges.size() + itr->second.size());
  edges.insert(edges.end(), itr->second.begin(), itr->second.end());
}

void IRoadGraph::GetFakeIngoingEdges(Junction const & junction, TEdgeVector & edges) const
{
  size_t const oldSize = edges.size();
  GetFakeOutgoingEdges(junction, edges);
  ReverseEdges(oldSize, edges);
}

void IRoadGraph::ResetFakes()
{
  m_outgoingEdges.clear();
}
void IRoadGraph::AddFakeEdges(Junction const & junction,
                              vector<pair<Edge, Junction>> const & vicinity)
{
  for (auto const & v : vicinity)
  {
    Edge const & closestEdge = v.first;
    Junction const p = v.second;

    if (p == closestEdge.GetStartJunction() || p == closestEdge.GetEndJunction())
    {
      // The point is mapped on the start junction of the edge or on the end junction of the edge:
      //        o M                                          o M
      //        ^                                            ^
      //        |                                            |
      //        |                                            |
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
      Edge edgeToSplit = closestEdge;

      vector<Edge> splittingToFakeEdges;
      if (HasBeenSplitToFakes(closestEdge, splittingToFakeEdges))
      {
        // Edge AB has already been split by some point Q and this point P
        // should split AB one more time
        //            o M
        //            ^
        //            |
        //            |
        // A o<-------x--------------x------------->o B
        //            P              Q

        auto const itr = FindEdgeContainingPoint(splittingToFakeEdges, p.GetPoint());
        CHECK(itr != splittingToFakeEdges.end(), ());

        edgeToSplit = *itr;
      }
      else
      {
        // The point P is mapped in the middle of the feature AB

        TEdgeVector & edgesA = m_outgoingEdges[edgeToSplit.GetStartJunction()];
        if (edgesA.empty())
          GetRegularOutgoingEdges(edgeToSplit.GetStartJunction(), edgesA);

        TEdgeVector & edgesB = m_outgoingEdges[edgeToSplit.GetEndJunction()];
        if (edgesB.empty())
          GetRegularOutgoingEdges(edgeToSplit.GetEndJunction(), edgesB);
      }

      //            o M
      //            ^
      //            |
      //            |
      // A o<-------x------->o B
      //            P
      // Here AB is the edge to split, M is a junction and P is the projection of M on AB,

      // Edge AB is split into two fake edges AP and PB (similarly BA edge has been split into two
      // fake edges BP and PA). In the result graph edges AB and BA are redundant, therefore edges AB and BA are
      // replaced by fake edges AP + PB and BP + PA.

      Edge const ab = edgeToSplit;

      Edge const pa(ab.GetFeatureId(), false /* forward */, ab.GetSegId(), p, ab.GetStartJunction());
      Edge const pb(ab.GetFeatureId(), true /* forward */, ab.GetSegId(), p, ab.GetEndJunction());
      Edge const pm = Edge::MakeFake(p, junction);

      // Add outgoing edges to point P.
      TEdgeVector & edgesP = m_outgoingEdges[p];
      edgesP.push_back(pa);
      edgesP.push_back(pb);
      edgesP.push_back(pm);

      // Add outgoing edges for point M.
      m_outgoingEdges[junction].push_back(pm.GetReverseEdge());

      // Replace AB edge with AP edge.
      TEdgeVector & edgesA = m_outgoingEdges[pa.GetEndJunction()];
      Edge const ap = pa.GetReverseEdge();
      edgesA.erase(remove_if(edgesA.begin(), edgesA.end(), [&](Edge const & e) { return e.SameRoadSegmentAndDirection(ap); }), edgesA.end());
      edgesA.push_back(ap);

      // Replace BA edge with BP edge.
      TEdgeVector & edgesB = m_outgoingEdges[pb.GetEndJunction()];
      Edge const bp = pb.GetReverseEdge();
      edgesB.erase(remove_if(edgesB.begin(), edgesB.end(), [&](Edge const & e) { return e.SameRoadSegmentAndDirection(bp); }), edgesB.end());
      edgesB.push_back(bp);
    }
  }

  // m_outgoingEdges may contain duplicates. Remove them.
  for (auto & m : m_outgoingEdges)
  {
    TEdgeVector & edges = m.second;
    sort(edges.begin(), edges.end());
    edges.erase(unique(edges.begin(), edges.end()), edges.end());
  }
}

bool IRoadGraph::HasBeenSplitToFakes(Edge const & edge, vector<Edge> & fakeEdges) const
{
  vector<Edge> tmp;

  if (m_outgoingEdges.find(edge.GetStartJunction()) == m_outgoingEdges.end() ||
      m_outgoingEdges.find(edge.GetEndJunction()) == m_outgoingEdges.end())
    return false;

  auto i = m_outgoingEdges.end();
  Junction junction = edge.GetStartJunction();
  while (m_outgoingEdges.end() != (i = m_outgoingEdges.find(junction)))
  {
    auto const & edges = i->second;

    auto const j = find_if(edges.begin(), edges.end(), [&edge](Edge const & e) { return e.SameRoadSegmentAndDirection(edge); });
    if (j == edges.end())
    {
      ASSERT(fakeEdges.empty(), ());
      return false;
    }

    tmp.push_back(*j);

    junction = j->GetEndJunction();
    if (junction == edge.GetEndJunction())
      break;
  }
  if (i == m_outgoingEdges.end())
    return false;

  if (tmp.empty())
    return false;

  fakeEdges.swap(tmp);
  return true;
}

double IRoadGraph::GetSpeedKMPH(Edge const & edge) const
{
  double const speedKMPH = (edge.IsFake() ? GetMaxSpeedKMPH() : GetSpeedKMPH(edge.GetFeatureId()));
  ASSERT(speedKMPH <= GetMaxSpeedKMPH(), ());
  return speedKMPH;
}

void IRoadGraph::GetEdgeTypes(Edge const & edge, feature::TypesHolder & types) const
{
  if (edge.IsFake())
    types = feature::TypesHolder(feature::GEOM_LINE);
  else
    GetFeatureTypes(edge.GetFeatureId(), types);
}

IRoadGraph::RoadInfo MakeRoadInfoForTesting(bool bidirectional, double speedKMPH,
                                            initializer_list<m2::PointD> const & points)
{
  IRoadGraph::RoadInfo ri(bidirectional, speedKMPH, {});
  for (auto const & p : points)
    ri.m_junctions.emplace_back(MakeJunctionForTesting(p));

  return ri;
}
}  // namespace routing
