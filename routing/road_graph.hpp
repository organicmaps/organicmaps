#pragma once

#include "geometry/point2d.hpp"

#include "base/string_utils.hpp"

#include "indexer/feature_data.hpp"

#include "std/initializer_list.hpp"
#include "std/map.hpp"
#include "std/vector.hpp"

namespace routing
{

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
public:
  static Edge MakeFake(Junction const & startJunction, Junction const & endJunction);

  Edge(FeatureID featureId, bool forward, uint32_t segId, Junction const & startJunction, Junction const & endJunction);
  Edge(Edge const &) = default;
  Edge & operator=(Edge const &) = default;

  inline FeatureID GetFeatureId() const { return m_featureId; }
  inline bool IsForward() const { return m_forward; }
  inline uint32_t GetSegId() const { return m_segId; }
  inline Junction const & GetStartJunction() const { return m_startJunction; }
  inline Junction const & GetEndJunction() const { return m_endJunction; }
  inline bool IsFake() const { return !m_featureId.IsValid(); }

  Edge GetReverseEdge() const;

  bool SameRoadSegmentAndDirection(Edge const & r) const;

  bool operator==(Edge const & r) const;
  bool operator<(Edge const & r) const;

private:
  friend string DebugPrint(Edge const & r);

  // Feature for which edge is defined.
  FeatureID m_featureId;

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

    void operator()(FeatureID const & featureId, RoadInfo const & roadInfo);

  private:
    m2::PointD const m_cross;
    TEdgeVector & m_outgoingEdges;
  };

  virtual ~IRoadGraph() = default;

  /// Finds all nearest outgoing edges, that route to the junction.
  void GetOutgoingEdges(Junction const & junction, TEdgeVector & edges) const;

  /// Finds all nearest ingoing edges, that route to the junction.
  void GetIngoingEdges(Junction const & junction, TEdgeVector & edges) const;

  /// Removes all fake turns and vertices from the graph.
  void ResetFakes();

  /// Adds fake edges from fake position rp to real vicinity
  /// positions.
  void AddFakeEdges(Junction const & junction, vector<pair<Edge, m2::PointD>> const & vicinities);

  /// Returns RoadInfo for a road corresponding to featureId.
  virtual RoadInfo GetRoadInfo(FeatureID const & featureId) const = 0;

  /// Returns speed in KM/H for a road corresponding to featureId.
  virtual double GetSpeedKMPH(FeatureID const & featureId) const = 0;

  /// Returns speed in KM/H for a road corresponding to edge.
  double GetSpeedKMPH(Edge const & edge) const;

  /// Returns max speed in KM/H
  virtual double GetMaxSpeedKMPH() const = 0;

  /// Calls edgesLoader on each feature which is close to cross.
  virtual void ForEachFeatureClosestToCross(m2::PointD const & cross,
                                            CrossEdgesLoader & edgesLoader) const = 0;

  /// Finds the closest edges to the point.
  /// @return Array of pairs of Edge and projection point on the Edge. If there is no the closest edges
  /// then returns empty array.
  virtual void FindClosestEdges(m2::PointD const & point, uint32_t count,
                                vector<pair<Edge, m2::PointD>> & vicinities) const = 0;

  /// @return Types for the specified feature
  virtual void GetFeatureTypes(FeatureID const & featureId, feature::TypesHolder & types) const = 0;

  /// @return Types for the specified edge
  void GetEdgeTypes(Edge const & edge, feature::TypesHolder & types) const;

  /// Clear all temporary buffers.
  virtual void ClearState() {}

private:
  /// Finds all outgoing regular (non-fake) edges for junction.
  void GetRegularOutgoingEdges(Junction const & junction, TEdgeVector & edges) const;

  /// Determines if the edge has been split by fake edges and if yes returns these fake edges.
  bool HasBeenSplitToFakes(Edge const & edge, vector<Edge> & fakeEdges) const;

  // Map of outgoing edges for junction
  map<Junction, TEdgeVector> m_outgoingEdges;
};

}  // namespace routing
