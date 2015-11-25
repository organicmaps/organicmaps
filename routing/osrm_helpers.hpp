#pragma once

#include "geometry/point2d.hpp"

#include "osrm2feature_map.hpp"
#include "osrm_data_facade.hpp"
#include "osrm_engine.hpp"
#include "routing_mapping.hpp"

#include "indexer/index.hpp"

namespace routing
{
namespace helpers
{
/// A helper class that prepares structures for OSRM by a geometry near a point.
class Point2PhantomNode
{
public:
  Point2PhantomNode(RoutingMapping const & mapping, Index const & index,
                    m2::PointD const & direction)
    : m_direction(direction), m_index(index), m_routingMapping(mapping)
  {
  }

  struct Candidate
  {
    // Square distance from point to geometry in meters.
    double m_dist;
    uint32_t m_segIdx;
    uint32_t m_fid;
    m2::PointD m_point;

    Candidate() : m_dist(numeric_limits<double>::max()),
                  m_segIdx(numeric_limits<uint32_t>::max()), m_fid(kInvalidFid)
    {}
  };

  // Finds nearest segment to a feature geometry.
  static void FindNearestSegment(FeatureType const & ft, m2::PointD const & point, Candidate & res);

  // Sets point from where weights are calculated.
  void SetPoint(m2::PointD const & pt) { m_point = pt; }

  // Returns true if there are candidate features for routing tasks.
  bool HasCandidates() const { return !m_candidates.empty(); }

  // Functor, for getting features from a index foreach method.
  void operator()(FeatureType const & ft);

  /// Makes OSRM tasks result vector.
  void MakeResult(vector<FeatureGraphNode> & res, size_t maxCount);

private:
  // Calculates distance in meters on the feature from startPoint to endPoint.
  double CalculateDistance(FeatureType const & feature, size_t const startPoint,
                           size_t const endPoint) const;

  /// Calculates part of a node weight in the OSRM format. Projection point @segPt divides node on
  /// two parts. So we find weight of a part, set by the @calcFromRight parameter.
  void CalculateWeight(OsrmMappingTypes::FtSeg const & seg, m2::PointD const & segPt,
                       NodeID const & nodeId, int & weight, int & offset) const;

  /// Returns minimal weight of the node.
  EdgeWeight GetMinNodeWeight(NodeID node, m2::PointD const & point) const;

  /// Calculates weights and offsets section of the routing tasks.
  void CalculateWeights(FeatureGraphNode & node) const;

  m2::PointD m_point;
  m2::PointD const m_direction;
  buffer_vector<Candidate, 128> m_candidates;
  Index const & m_index;
  RoutingMapping const & m_routingMapping;

  DISALLOW_COPY(Point2PhantomNode);
};

/// Class-getter for finding OSRM nodes near geometry point.
class Point2Node
{
  RoutingMapping const & m_routingMapping;
  vector<NodeID> & m_nodeIds;

public:
  Point2Node(RoutingMapping const & routingMapping, vector<NodeID> & nodeID)
    : m_routingMapping(routingMapping), m_nodeIds(nodeID)
  {
  }

  // Functor, for getting features from a index foreach method.
  void operator()(FeatureType const & ft);

  DISALLOW_COPY_AND_MOVE(Point2Node);
};
}  // namespace helpers
}  // namespace routing
