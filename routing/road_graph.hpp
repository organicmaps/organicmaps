#pragma once

#include "routing/base/graph.hpp"

#include "geometry/point2d.hpp"

#include "base/string_utils.hpp"

#include "std/initializer_list.hpp"
#include "std/limits.hpp"
#include "std/map.hpp"
#include "std/vector.hpp"

namespace routing
{

class Route;

/// The Junction class represents a node description on a road network graph
class Junction
{
public:
  Junction();
  Junction(m2::PointD const & point);
  Junction(Junction const &) = default;
  Junction & operator=(Junction const &) = default;

  inline bool operator==(Junction const & r) const { return m_point == r.m_point; }
  inline bool operator<(Junction const & r) const { return m_point < r.m_point; }

  m2::PointD const & GetPoint() const { return m_point; }

private:
  friend string DebugPrint(Junction const & r);

  // Point of the junction
  m2::PointD m_point;
};

/// The Edge class represents an edge description on a road network graph
class Edge
{
private:
  static constexpr uint32_t kFakeFeatureId = numeric_limits<uint32_t>::max();

public:
  static Edge MakeFake(Junction const & startJunction, Junction const & endJunction);

  Edge(uint32_t featureId, bool forward, size_t segId, Junction const & startJunction, Junction const & endJunction);
  Edge(Edge const &) = default;
  Edge & operator=(Edge const &) = default;

  inline uint32_t GetFeatureId() const { return m_featureId; }
  inline bool IsForward() const { return m_forward; }
  inline uint32_t GetSegId() const { return m_segId; }
  inline Junction const & GetStartJunction() const { return m_startJunction; }
  inline Junction const & GetEndJunction() const { return m_endJunction; }
  inline bool IsFake() const { return m_featureId == kFakeFeatureId; }

  Edge GetReverseEdge() const;

  bool SameRoadSegmentAndDirection(Edge const & r) const;

  bool operator==(Edge const & r) const;
  bool operator<(Edge const & r) const;

private:
  friend string DebugPrint(Edge const & r);

  // Feature on which position is defined.
  uint32_t m_featureId;

  // Is the feature along the road.
  bool m_forward;

  // Ordinal number of the segment on the road.
  uint32_t m_segId;

  // Start junction of the segment on the road.
  Junction m_startJunction;

  // End junction of the segment on the road.
  Junction m_endJunction;
};

class IRoadGraph
{
public:
  typedef vector<Junction> TJunctionVector;
  typedef vector<Edge> TEdgeVector;

  /// This struct contains the part of a feature's metadata that is
  /// relevant for routing.
  struct RoadInfo
  {
    RoadInfo();
    RoadInfo(RoadInfo && ri);
    RoadInfo(bool bidirectional, double speedKMPH, initializer_list<m2::PointD> const & points);
    RoadInfo(RoadInfo const &) = default;
    RoadInfo & operator=(RoadInfo const &) = default;

    buffer_vector<m2::PointD, 32> m_points;
    double m_speedKMPH;
    bool m_bidirectional;
  };

  /// This class is responsible for loading edges in a cross.
  /// It loades only outgoing edges.
  class CrossEdgesLoader
  {
  public:
    CrossEdgesLoader(m2::PointD const & cross, TEdgeVector & outgoingEdges);

    void operator()(uint32_t featureId, RoadInfo const & roadInfo);

  private:
    m2::PointD const m_cross;
    TEdgeVector & m_outgoingEdges;
  };

  virtual ~IRoadGraph() = default;

  /// Construct route by road positions (doesn't include first and last section).
  void ReconstructPath(TJunctionVector const & junctions, Route & route);

  /// Finds all nearest outgoing edges, that route to the junction.
  void GetOutgoingEdges(Junction const & junction, TEdgeVector & edges);

  /// Finds all nearest ingoing edges, that route to the junction.
  void GetIngoingEdges(Junction const & junction, TEdgeVector & edges);

  /// Removes all fake turns and vertices from the graph.
  void ResetFakes();

  /// Adds fake edges from fake position rp to real vicinity
  /// positions.
  void AddFakeEdges(Junction const & junction, vector<pair<Edge, m2::PointD>> const & vicinities);

  /// Returns RoadInfo for a road corresponding to featureId.
  virtual RoadInfo GetRoadInfo(uint32_t featureId) = 0;

  /// Returns speed in KM/H for a road corresponding to featureId.
  virtual double GetSpeedKMPH(uint32_t featureId) = 0;

  /// Returns max speed in KM/H
  virtual double GetMaxSpeedKMPH() = 0;

  /// Calls edgesLoader on each feature which is close to cross.
  virtual void ForEachFeatureClosestToCross(m2::PointD const & cross,
                                            CrossEdgesLoader & edgesLoader) = 0;

private:
  /// Finds all outgoing regular (non-fake) edges for junction.
  void GetRegularOutgoingEdges(Junction const & junction, TEdgeVector & edges);

  /// Determines if the edge has been split by fake edges and if yes returns these fake edges.
  bool HasBeenSplitToFakes(Edge const & edge, vector<Edge> & fakeEdges) const;

  // Map of outgoing edges for junction
  map<Junction, TEdgeVector> m_outgoingEdges;
};

/// A class which represents an weighted edge used by RoadGraph.
struct WeightedEdge
{
  WeightedEdge(Junction const & target, double weight) : target(target), weight(weight) {}

  inline Junction const & GetTarget() const { return target; }

  inline double GetWeight() const { return weight; }

  Junction const target;
  double const weight;
};

/// A wrapper around IGraph, which makes it possible to use IRoadGraph
/// with routing algorithms.
class RoadGraph : public Graph<Junction, WeightedEdge, RoadGraph>
{
public:
  RoadGraph(IRoadGraph & roadGraph);

private:
  friend class Graph<Junction, WeightedEdge, RoadGraph>;

  // Returns speed in M/S for specified edge
  double GetSpeedMPS(Edge const & edge) const;

  // Graph<Junction, WeightedEdge, RoadGraph> implementation:
  void GetOutgoingEdgesListImpl(Junction const & v, vector<WeightedEdge> & adj) const;
  void GetIngoingEdgesListImpl(Junction const & v, vector<WeightedEdge> & adj) const;
  double HeuristicCostEstimateImpl(Junction const & v, Junction const & w) const;

  IRoadGraph & m_roadGraph;
};

}  // namespace routing
