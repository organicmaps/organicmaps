#pragma once

#include "routing/segment.hpp"

#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"

#include "geometry/point2d.hpp"

#include "base/string_utils.hpp"

#include "indexer/feature_altitude.hpp"
#include "indexer/feature_data.hpp"

#include <initializer_list>
#include <map>
#include <vector>

namespace routing
{
double constexpr kPointsEqualEpsilon = 1e-6;

/// The Junction class represents a node description on a road network graph
class Junction
{
public:
  Junction();
  Junction(m2::PointD const & point, feature::TAltitude altitude);
  Junction(Junction const &) = default;
  Junction & operator=(Junction const &) = default;

  inline bool operator==(Junction const & r) const { return m_point == r.m_point; }
  inline bool operator!=(Junction const & r) const { return !(*this == r); }
  inline bool operator<(Junction const & r) const { return m_point < r.m_point; }

  inline m2::PointD const & GetPoint() const { return m_point; }
  inline feature::TAltitude GetAltitude() const { return m_altitude; }

private:
  friend string DebugPrint(Junction const & r);

  // Point of the junction
  m2::PointD m_point;
  feature::TAltitude m_altitude;
};

inline Junction MakeJunctionForTesting(m2::PointD const & point)
{
  return Junction(point, feature::kDefaultAltitudeMeters);
}

inline bool AlmostEqualAbs(Junction const & lhs, Junction const & rhs)
{
  return base::AlmostEqualAbs(lhs.GetPoint(), rhs.GetPoint(), kPointsEqualEpsilon);
}

/// The Edge class represents an edge description on a road network graph
class Edge
{
  enum class Type
  {
    Real,               // An edge that corresponds to some real segment.
    FakeWithRealPart,   // A fake edge that is a part of some real segment.
    FakeWithoutRealPart  // A fake edge that is not part of any real segment.
  };

public:
  Edge() = default;
  Edge(Edge const &) = default;
  Edge & operator=(Edge const &) = default;

  static Edge MakeReal(FeatureID const & featureId, bool forward, uint32_t segId,
                       Junction const & startJunction, Junction const & endJunction);
  static Edge MakeFakeWithRealPart(FeatureID const & featureId, bool forward, uint32_t segId,
                                   Junction const & startJunction, Junction const & endJunctio);
  static Edge MakeFake(Junction const & startJunction, Junction const & endJunction);
  static Edge MakeFake(Junction const & startJunction, Junction const & endJunction,
                       Edge const & prototype);

  inline FeatureID GetFeatureId() const { return m_featureId; }
  inline bool IsForward() const { return m_forward; }
  inline uint32_t GetSegId() const { return m_segId; }
  inline Junction const & GetStartJunction() const { return m_startJunction; }
  inline Junction const & GetEndJunction() const { return m_endJunction; }
  inline m2::PointD const & GetStartPoint() const { return m_startJunction.GetPoint(); }
  inline m2::PointD const & GetEndPoint() const { return m_endJunction.GetPoint(); }
  inline bool IsFake() const { return  m_type != Type::Real; }
  inline bool HasRealPart() const { return m_type != Type::FakeWithoutRealPart; }
  inline m2::PointD GetDirection() const
  {
    return GetEndJunction().GetPoint() - GetStartJunction().GetPoint();
  }

  Edge GetReverseEdge() const;

  bool SameRoadSegmentAndDirection(Edge const & r) const;

  bool operator==(Edge const & r) const;
  bool operator!=(Edge const & r) const { return !(*this == r); }
  bool operator<(Edge const & r) const;

private:
  Edge(Type type, FeatureID const & featureId, bool forward, uint32_t segId,
       Junction const & startJunction, Junction const & endJunction);

  friend string DebugPrint(Edge const & r);

  Type m_type = Type::FakeWithoutRealPart;

  // Feature for which edge is defined.
  FeatureID m_featureId;

  // Is the feature along the road.
  bool m_forward = true;

  // Ordinal number of the segment on the road.
  uint32_t m_segId = 0;

  // Start junction of the segment on the road.
  Junction m_startJunction;

  // End junction of the segment on the road.
  Junction m_endJunction;

  // Note. If |m_forward| == true index of |m_startJunction| point at the feature |m_featureId|
  // is less than index |m_endJunction|.
  // If |m_forward| == false index of |m_startJunction| point at the feature |m_featureId|
  // is more than index |m_endJunction|.
};

class RoadGraphBase
{
public:
  typedef vector<Edge> TEdgeVector;

  /// Finds all nearest outgoing edges, that route to the junction.
  virtual void GetOutgoingEdges(Junction const & junction, TEdgeVector & edges) const = 0;

  /// Finds all nearest ingoing edges, that route to the junction.
  virtual void GetIngoingEdges(Junction const & junction, TEdgeVector & edges) const = 0;

  /// Returns max speed in KM/H
  virtual double GetMaxSpeedKMpH() const = 0;

  /// @return Types for the specified edge
  virtual void GetEdgeTypes(Edge const & edge, feature::TypesHolder & types) const = 0;

  /// @return Types for specified junction
  virtual void GetJunctionTypes(Junction const & junction, feature::TypesHolder & types) const = 0;

  virtual void GetRouteEdges(TEdgeVector & routeEdges) const;
  virtual void GetRouteSegments(std::vector<Segment> & segments) const;
};

class IRoadGraph : public RoadGraphBase
{
public:
  // CheckGraphConnectivity() types aliases:
  using Vertex = Junction;
  using Edge = routing::Edge;
  using Weight = double;
  using JunctionVec = buffer_vector<Junction, 32>;

  enum class Mode
  {
    ObeyOnewayTag,
    IgnoreOnewayTag,
  };

  /// This struct contains the part of a feature's metadata that is
  /// relevant for routing.
  struct RoadInfo
  {
    RoadInfo();
    RoadInfo(RoadInfo && ri);
    RoadInfo(bool bidirectional, double speedKMPH, std::initializer_list<Junction> const & points);
    RoadInfo(RoadInfo const &) = default;
    RoadInfo & operator=(RoadInfo const &) = default;

    JunctionVec m_junctions;
    double m_speedKMPH;
    bool m_bidirectional;
  };

  /// This class is responsible for loading edges in a cross.
  class ICrossEdgesLoader
  {
  public:
    ICrossEdgesLoader(Junction const & cross, IRoadGraph::Mode mode, TEdgeVector & edges)
      : m_cross(cross), m_mode(mode), m_edges(edges)
    {
    }

    virtual ~ICrossEdgesLoader() = default;

    void operator()(FeatureID const & featureId, JunctionVec const & junctions,
                    bool bidirectional)
    {
      LoadEdges(featureId, junctions, bidirectional);
    }

  private:
    virtual void LoadEdges(FeatureID const & featureId, JunctionVec const & junctions,
                           bool bidirectional) = 0;

  protected:
    template <typename TFn>
    void ForEachEdge(JunctionVec const & junctions, TFn && fn)
    {
      for (size_t i = 0; i < junctions.size(); ++i)
      {
        if (!base::AlmostEqualAbs(m_cross.GetPoint(), junctions[i].GetPoint(), kPointsEqualEpsilon))
          continue;

        if (i + 1 < junctions.size())
        {
          // Head of the edge.
          // m_cross
          //     o------------>o
          fn(i, junctions[i + 1], true /* forward */);
        }
        if (i > 0)
        {
          // Tail of the edge.
          //                m_cross
          //     o------------>o
          fn(i - 1, junctions[i - 1], false /* backward */);
        }
      }
    }

    Junction const m_cross;
    IRoadGraph::Mode const m_mode;
    TEdgeVector & m_edges;
  };

  class CrossOutgoingLoader : public ICrossEdgesLoader
  {
  public:
    CrossOutgoingLoader(Junction const & cross, IRoadGraph::Mode mode, TEdgeVector & edges)
      : ICrossEdgesLoader(cross, mode, edges)
    {
    }

  private:
    // ICrossEdgesLoader overrides:
    void LoadEdges(FeatureID const & featureId, JunctionVec const & junctions,
                   bool bidirectional) override;
  };

  class CrossIngoingLoader : public ICrossEdgesLoader
  {
  public:
    CrossIngoingLoader(Junction const & cross, IRoadGraph::Mode mode, TEdgeVector & edges)
      : ICrossEdgesLoader(cross, mode, edges)
    {
    }

  private:
    // ICrossEdgesLoader overrides:
    void LoadEdges(FeatureID const & featureId, JunctionVec const & junctions,
                   bool bidirectional) override;
  };

  virtual ~IRoadGraph() = default;

  void GetOutgoingEdges(Junction const & junction, TEdgeVector & edges) const override;

  void GetIngoingEdges(Junction const & junction, TEdgeVector & edges) const override;

  /// Removes all fake turns and vertices from the graph.
  void ResetFakes();

  /// Adds fake edges from fake position rp to real vicinity
  /// positions.
  void AddFakeEdges(Junction const & junction, vector<pair<Edge, Junction>> const & vicinities);
  void AddOutgoingFakeEdge(Edge const & e);
  void AddIngoingFakeEdge(Edge const & e);

  /// Returns RoadInfo for a road corresponding to featureId.
  virtual RoadInfo GetRoadInfo(FeatureID const & featureId, SpeedParams const & speedParams) const = 0;

  /// Returns speed in KM/H for a road corresponding to featureId.
  virtual double GetSpeedKMpH(FeatureID const & featureId, SpeedParams const & speedParams) const = 0;

  /// Returns speed in KM/H for a road corresponding to edge.
  double GetSpeedKMpH(Edge const & edge, SpeedParams const & speedParams) const;

  /// Calls edgesLoader on each feature which is close to cross.
  virtual void ForEachFeatureClosestToCross(m2::PointD const & cross,
                                            ICrossEdgesLoader & edgesLoader) const = 0;

  /// Finds the closest edges to the point.
  /// @return Array of pairs of Edge and projection point on the Edge. If there is no the closest edges
  /// then returns empty array.
  virtual void FindClosestEdges(m2::PointD const & point, uint32_t count,
                                vector<pair<Edge, Junction>> & vicinities) const = 0;

  /// @return Types for the specified feature
  virtual void GetFeatureTypes(FeatureID const & featureId, feature::TypesHolder & types) const = 0;

  /// @return Types for the specified edge
  virtual void GetEdgeTypes(Edge const & edge, feature::TypesHolder & types) const override;

  virtual IRoadGraph::Mode GetMode() const = 0;

  /// Clear all temporary buffers.
  virtual void ClearState() {}

  /// \brief Finds all outgoing regular (non-fake) edges for junction.
  void GetRegularOutgoingEdges(Junction const & junction, TEdgeVector & edges) const;
  /// \brief Finds all ingoing regular (non-fake) edges for junction.
  void GetRegularIngoingEdges(Junction const & junction, TEdgeVector & edges) const;
  /// \brief Finds all outgoing fake edges for junction.
  void GetFakeOutgoingEdges(Junction const & junction, TEdgeVector & edges) const;
  /// \brief Finds all ingoing fake edges for junction.
  void GetFakeIngoingEdges(Junction const & junction, TEdgeVector & edges) const;

private:
  void AddEdge(Junction const & j, Edge const & e, std::map<Junction, TEdgeVector> & edges);

  template <typename Fn>
  void ForEachFakeEdge(Fn && fn)
  {
    for (auto const & m : m_fakeIngoingEdges)
    {
      for (auto const & e : m.second)
        fn(e);
    }

    for (auto const & m : m_fakeOutgoingEdges)
    {
      for (auto const & e : m.second)
        fn(e);
    }
  }

  /// \note |m_fakeIngoingEdges| and |m_fakeOutgoingEdges| map junctions to sorted vectors.
  /// Items to these maps should be inserted with AddEdge() method only.
  std::map<Junction, TEdgeVector> m_fakeIngoingEdges;
  std::map<Junction, TEdgeVector> m_fakeOutgoingEdges;
};

string DebugPrint(IRoadGraph::Mode mode);

IRoadGraph::RoadInfo MakeRoadInfoForTesting(bool bidirectional, double speedKMPH,
                                            std::initializer_list<m2::PointD> const & points);

inline void JunctionsToPoints(vector<Junction> const & junctions, vector<m2::PointD> & points)
{
  points.resize(junctions.size());
  for (size_t i = 0; i < junctions.size(); ++i)
    points[i] = junctions[i].GetPoint();
}

inline void JunctionsToAltitudes(vector<Junction> const & junctions, feature::TAltitudes & altitudes)
{
  altitudes.resize(junctions.size());
  for (size_t i = 0; i < junctions.size(); ++i)
    altitudes[i] = junctions[i].GetAltitude();
}
}  // namespace routing
