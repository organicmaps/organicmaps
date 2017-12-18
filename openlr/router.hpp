#pragma once

#include "openlr/way_point.hpp"

#include "routing/road_graph.hpp"

#include "geometry/point2d.hpp"

#include "std/map.hpp"
#include "std/sstream.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace routing
{
class FeaturesRoadGraph;
}

namespace openlr
{
class RoadInfoGetter;

class Router final
{
public:
  Router(routing::FeaturesRoadGraph & graph, RoadInfoGetter & roadInfoGetter);

  bool Go(vector<WayPoint> const & points, double positiveOffsetM, double negativeOffsetM,
          vector<routing::Edge> & path);

private:
  struct Vertex final
  {
    Vertex() = default;
    Vertex(routing::Junction const & junction, routing::Junction const & stageStart,
           double stageStartDistance, size_t stage, bool bearingChecked);

    bool operator<(Vertex const & rhs) const;
    bool operator==(Vertex const & rhs) const;
    bool operator!=(Vertex const & rhs) const { return !(*this == rhs); }

    m2::PointD GetPoint() const { return m_junction.GetPoint(); }

    routing::Junction m_junction;
    routing::Junction m_stageStart;
    double m_stageStartDistance = 0.0;
    size_t m_stage = 0;
    bool m_bearingChecked = false;
  };

  friend string DebugPrint(Vertex const & u)
  {
    ostringstream os;
    os << "Vertex [ ";
    os << "junction: " << DebugPrint(u.m_junction) << ", ";
    os << "stageStart: " << DebugPrint(u.m_stageStart) << ", ";
    os << "stageStartDistance: " << u.m_stageStartDistance << ", ";
    os << "stage: " << u.m_stage << ", ";
    os << "bearingChecked: " << u.m_bearingChecked;
    os << " ]";
    return os.str();
  }

  struct Edge final
  {
    Edge() = default;
    Edge(Vertex const & u, Vertex const & v, routing::Edge const & raw, bool isSpecial);

    static Edge MakeNormal(Vertex const & u, Vertex const & v, routing::Edge const & raw);
    static Edge MakeSpecial(Vertex const & u, Vertex const & v);

    bool IsFake() const { return m_raw.IsFake(); }
    bool IsSpecial() const { return m_isSpecial; }

    pair<m2::PointD, m2::PointD> ToPair() const;
    pair<m2::PointD, m2::PointD> ToPairRev() const;

    Vertex m_u;
    Vertex m_v;
    routing::Edge m_raw;
    bool m_isSpecial = false;
  };

  friend string DebugPrint(Edge const & edge)
  {
    ostringstream os;
    os << "Edge [ ";
    os << "u: " << DebugPrint(edge.m_u) << ", ";
    os << "v: " << DebugPrint(edge.m_v) << ", ";
    os << "raw: " << DebugPrint(edge.m_raw) << ", ";
    os << "isSpecial: " << edge.m_isSpecial;
    os << " ]";
    return os.str();
  }

  using Links = map<Vertex, pair<Vertex, Edge>>;

  using RoadGraphEdgesGetter = void (routing::IRoadGraph::*)(
      routing::Junction const & junction, routing::IRoadGraph::TEdgeVector & edges) const;

  bool Init(vector<WayPoint> const & points, double positiveOffsetM, double negativeOffsetM);
  bool FindPath(vector<routing::Edge> & path);

  // Returns true if the bearing should be checked for |u|, if the
  // real passed distance from the source vertex is |distanceM|.
  bool NeedToCheckBearing(Vertex const & u, double distanceM) const;

  double GetPotential(Vertex const & u) const;

  // Returns true if |u| is located near portal to the next stage.
  // |pi| is the potential of |u|.
  bool NearNextStage(Vertex const & u, double pi) const;

  // Returns true if it's possible to move to the next stage from |u|.
  // |pi| is the potential of |u|.
  bool MayMoveToNextStage(Vertex const & u, double pi) const;

  // Returns true if |u| is a final vertex and the router may stop now.
  bool IsFinalVertex(Vertex const & u) const { return u.m_stage == m_pivots.size(); }

  double GetWeight(routing::Edge const & e) const
  {
    return MercatorBounds::DistanceOnEarth(e.GetStartJunction().GetPoint(),
                                           e.GetEndJunction().GetPoint());
  }

  double GetWeight(Edge const & e) const { return GetWeight(e.m_raw); }

  uint32_t GetReverseBearing(Vertex const & u, Links const & links) const;

  template <typename Fn>
  void ForEachEdge(Vertex const & u, bool outgoing, FunctionalRoadClass restriction, Fn && fn);

  void GetOutgoingEdges(routing::Junction const & u, routing::IRoadGraph::TEdgeVector & edges);
  void GetIngoingEdges(routing::Junction const & u, routing::IRoadGraph::TEdgeVector & edges);
  void GetEdges(routing::Junction const & u, RoadGraphEdgesGetter getRegular,
                RoadGraphEdgesGetter getFake,
                map<routing::Junction, routing::IRoadGraph::TEdgeVector> & cache,
                routing::IRoadGraph::TEdgeVector & edges);

  template <typename Fn>
  void ForEachNonFakeEdge(Vertex const & u, bool outgoing, FunctionalRoadClass restriction,
                          Fn && fn);

  template <typename Fn>
  void ForEachNonFakeClosestEdge(Vertex const & u, FunctionalRoadClass const restriction, Fn && fn);

  template <typename It>
  size_t FindPrefixLengthToConsume(It b, It const e, double lengthM);

  // Finds all edges that are on (u, v) and have the same direction as
  // (u, v).  Then, computes the fraction of the union of these edges
  // to the total length of (u, v).
  template <typename It>
  double GetCoverage(m2::PointD const & u, m2::PointD const & v, It b, It e);

  // Finds the longest prefix of [b, e) that covers edge (u, v).
  // Returns the fraction of the coverage to the length of the (u, v).
  template <typename It>
  double GetMatchingScore(m2::PointD const & u, m2::PointD const & v, It b, It e);

  // Finds the longest prefix of fake edges of [b, e) that have the
  // same stage as |stage|. If the prefix exists, passes its bounding
  // iterator to |fn|.
  template <typename It, typename Fn>
  void ForStagePrefix(It b, It e, size_t stage, Fn && fn);

  bool ReconstructPath(vector<Edge> & edges, vector<routing::Edge> & path);

  void FindSingleEdgeApproximation(vector<Edge> const & edges, vector<routing::Edge> & path);

  routing::FeaturesRoadGraph & m_graph;
  map<routing::Junction, routing::IRoadGraph::TEdgeVector> m_outgoingCache;
  map<routing::Junction, routing::IRoadGraph::TEdgeVector> m_ingoingCache;
  RoadInfoGetter & m_roadInfoGetter;

  vector<WayPoint> m_points;
  double m_positiveOffsetM;
  double m_negativeOffsetM;
  vector<vector<m2::PointD>> m_pivots;
  routing::Junction m_sourceJunction;
  routing::Junction m_targetJunction;
};
}  // namespace openlr
