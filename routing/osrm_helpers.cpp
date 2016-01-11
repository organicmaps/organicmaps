#include "car_model.hpp"
#include "osrm_helpers.hpp"

#include "indexer/scales.hpp"

#include "geometry/distance.hpp"

namespace routing
{
namespace helpers
{
// static
void Point2PhantomNode::FindNearestSegment(FeatureType const & ft, m2::PointD const & point,
                                           Candidate & res)
{
  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

  size_t const count = ft.GetPointsCount();
  uint32_t const featureId = ft.GetID().m_index;
  ASSERT_GREATER(count, 1, ());
  for (size_t i = 1; i < count; ++i)
  {
    m2::ProjectionToSection<m2::PointD> segProj;
    segProj.SetBounds(ft.GetPoint(i - 1), ft.GetPoint(i));

    m2::PointD const pt = segProj(point);
    double const d = point.SquareLength(pt);
    if (d < res.m_dist)
    {
      res.m_dist = d;
      res.m_fid = featureId;
      res.m_segIdx = static_cast<uint32_t>(i - 1);
      res.m_point = pt;
    }
  }
}

void Point2PhantomNode::operator()(FeatureType const & ft)
{
  if (!CarModel::Instance().IsRoad(ft))
    return;

  Candidate res;

  FindNearestSegment(ft, m_point, res);

  if (res.m_fid != kInvalidFid && !m_routingMapping.m_segMapping.GetNodeIdByFid(res.m_fid).empty())
    m_candidates.push_back(res);
}

double Point2PhantomNode::CalculateDistance(FeatureType const & ft,
                                            size_t const pointStart,
                                            size_t const pointEnd) const
{
  ASSERT_NOT_EQUAL(pointStart, pointEnd, ());

  double distMeters = 0.0;
  size_t const n = max(pointStart, pointEnd);
  size_t i = min(pointStart, pointEnd) + 1;
  do
  {
    distMeters += MercatorBounds::DistanceOnEarth(ft.GetPoint(i - 1), ft.GetPoint(i));
    ++i;
  } while (i <= n);

  return distMeters;
}

void Point2PhantomNode::CalculateWeight(OsrmMappingTypes::FtSeg const & seg,
                                        m2::PointD const & segPt, NodeID const & nodeId,
                                        int & weight, int & offset) const
{
  // nodeId can be INVALID_NODE_ID when reverse node is absent. This node has no weight.
  if (nodeId == INVALID_NODE_ID)
  {
    offset = 0;
    weight = 0;
    return;
  }

  Index::FeaturesLoaderGuard loader(m_index, m_routingMapping.GetMwmId());

  // Offset is measured in milliseconds. We don't know about speed restrictions on the road.
  // So we find it by a whole edge weight.
  // Distance from the node border to the projection point is in meters.
  double distanceM = 0.;
  // Whole node distance in meters.
  double fullDistanceM = 0.;
  // Minimal OSRM edge weight in milliseconds.
  EdgeWeight minWeight = 0;

  auto const range = m_routingMapping.m_segMapping.GetSegmentsRange(nodeId);
  OsrmMappingTypes::FtSeg segment;

  bool foundSeg = false;
  for (size_t segmentIndex = range.first; segmentIndex != range.second; ++segmentIndex)
  {
    m_routingMapping.m_segMapping.GetSegmentByIndex(segmentIndex, segment);
    if (!segment.IsValid())
      continue;

    FeatureType ft;
    loader.GetFeatureByIndex(segment.m_fid, ft);
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    // Find whole edge weight by node outgoing point.
    if (segmentIndex == range.second - 1)
      minWeight = GetMinNodeWeight(nodeId, ft.GetPoint(segment.m_pointEnd));

    // Calculate distances.
    double distance = CalculateDistance(ft, segment.m_pointStart, segment.m_pointEnd);
    fullDistanceM += distance;
    if (foundSeg)
      continue;

    if (segment.m_fid == seg.m_fid && OsrmMappingTypes::IsInside(segment, seg))
    {
      auto const splittedSegment = OsrmMappingTypes::SplitSegment(segment, seg);
      distanceM += CalculateDistance(ft, splittedSegment.m_pointStart, splittedSegment.m_pointEnd);
      // node.m_seg always forward ordered (m_pointStart < m_pointEnd)
      distanceM -= MercatorBounds::DistanceOnEarth(ft.GetPoint(seg.m_pointEnd), segPt);

      foundSeg = true;
    }
    else
    {
      distanceM += distance;
    }
  }

  ASSERT(foundSeg, ("Intersection not found!"));
  ASSERT_GREATER(fullDistanceM, 0, ("No valid segments on the edge."));
  double const ratio = (fullDistanceM == 0) ? 0 : distanceM / fullDistanceM;
  ASSERT_LESS_OR_EQUAL(ratio, 1., ());

  // OSRM calculates edge weight form start to user point how offset + weight.
  // But it doesn't place info about start and end edge result weight into result structure.
  // So we store whole edge weight into offset and calculates this weights at a postprocessing step.
  offset = minWeight;
  weight = max(static_cast<int>(minWeight * ratio), 0) - minWeight;
}

EdgeWeight Point2PhantomNode::GetMinNodeWeight(NodeID node, m2::PointD const & point) const
{
  static double const kInfinity = numeric_limits<EdgeWeight>::max();
  static double const kReadCrossRadiusMercator = 1.0E-4;
  EdgeWeight minWeight = kInfinity;
  // Geting nodes by geometry.
  vector<NodeID> geomNodes;
  Point2Node p2n(m_routingMapping, geomNodes);

  m_index.ForEachInRectForMWM(p2n,
    m2::RectD(point.x - kReadCrossRadiusMercator, point.y - kReadCrossRadiusMercator,
              point.x + kReadCrossRadiusMercator, point.y + kReadCrossRadiusMercator),
    scales::GetUpperScale(), m_routingMapping.GetMwmId());

  sort(geomNodes.begin(), geomNodes.end());
  geomNodes.erase(unique(geomNodes.begin(), geomNodes.end()), geomNodes.end());

  // Filtering virtual edges.
  for (EdgeID const & e : m_routingMapping.m_dataFacade.GetAdjacentEdgeRange(node))
  {
    QueryEdge::EdgeData const data = m_routingMapping.m_dataFacade.GetEdgeData(e, node);
    if (data.forward && !data.shortcut)
      minWeight = min(minWeight, data.distance);
  }

  for (NodeID const & adjacentNode : geomNodes)
  {
    if (adjacentNode == node)
      continue;
    for (EdgeID const & e : m_routingMapping.m_dataFacade.GetAdjacentEdgeRange(adjacentNode))
    {
      if (m_routingMapping.m_dataFacade.GetTarget(e) != node)
        continue;
      QueryEdge::EdgeData const data = m_routingMapping.m_dataFacade.GetEdgeData(e, adjacentNode);
      if (!data.shortcut && data.backward)
        minWeight = min(minWeight, data.distance);
    }
  }
  // If we can't determine edge weight.
  if (minWeight == kInfinity)
    return 0;
  return minWeight;
}

void Point2PhantomNode::MakeResult(vector<FeatureGraphNode> & res, size_t maxCount)
{
  vector<OsrmMappingTypes::FtSeg> segments;

  sort(m_candidates.begin(), m_candidates.end(), [](Candidate const & r1, Candidate const & r2)
       {
         return (r1.m_dist < r2.m_dist);
       });

  size_t const n = min(m_candidates.size(), maxCount);
  segments.resize(n);

  for (size_t j = 0; j < n; ++j)
  {
    OsrmMappingTypes::FtSeg & seg = segments[j];
    Candidate const & c = m_candidates[j];

    seg.m_fid = c.m_fid;
    seg.m_pointStart = c.m_segIdx;
    seg.m_pointEnd = c.m_segIdx + 1;
  }

  OsrmFtSegMapping::OsrmNodesT nodes;
  m_routingMapping.m_segMapping.GetOsrmNodes(segments, nodes);

  res.clear();
  res.resize(maxCount);

  for (size_t j = 0; j < maxCount; ++j)
  {
    if (!segments[j].IsValid())
      continue;

    auto it = nodes.find(segments[j].Store());
    if (it == nodes.cend())
      continue;

    FeatureGraphNode & node = res[j];

    if (!m_direction.IsAlmostZero())
    {
      // Filter income nodes by direction mode
      OsrmMappingTypes::FtSeg const & node_seg = segments[j];
      FeatureType feature;
      Index::FeaturesLoaderGuard loader(m_index, m_routingMapping.GetMwmId());
      loader.GetFeatureByIndex(node_seg.m_fid, feature);
      feature.ParseGeometry(FeatureType::BEST_GEOMETRY);
      m2::PointD const featureDirection = feature.GetPoint(node_seg.m_pointEnd) - feature.GetPoint(node_seg.m_pointStart);
      bool const sameDirection = (m2::DotProduct(featureDirection, m_direction) > 0);
      if (sameDirection)
      {
        node.node.forward_node_id = it->second.first;
        node.node.reverse_node_id = INVALID_NODE_ID;
      }
      else
      {
        node.node.forward_node_id = INVALID_NODE_ID;
        node.node.reverse_node_id = it->second.second;
      }
    }
    else
    {
      node.node.forward_node_id = it->second.first;
      node.node.reverse_node_id = it->second.second;
    }

    node.segment = segments[j];
    node.segmentPoint = m_candidates[j].m_point;
    node.mwmId = m_routingMapping.GetMwmId();

    CalculateWeights(node);
  }
  res.erase(remove_if(res.begin(), res.end(),
                      [](FeatureGraphNode const & f)
                      {
                        return !f.mwmId.IsAlive();
                      }),
            res.end());
}

void Point2PhantomNode::CalculateWeights(FeatureGraphNode & node) const
{
    // Need to initialize weights for correct work of PhantomNode::GetForwardWeightPlusOffset
    // and PhantomNode::GetReverseWeightPlusOffset.
  CalculateWeight(node.segment, node.segmentPoint, node.node.forward_node_id,
                  node.node.forward_weight, node.node.forward_offset);
  CalculateWeight(node.segment, node.segmentPoint, node.node.reverse_node_id,
                  node.node.reverse_weight, node.node.reverse_offset);
}

void Point2Node::operator()(FeatureType const & ft)
{
  if (!CarModel::Instance().IsRoad(ft))
    return;
  uint32_t const featureId = ft.GetID().m_index;
  for (auto const & n : m_routingMapping.m_segMapping.GetNodeIdByFid(featureId))
    m_nodeIds.push_back(n);
}
}  // namespace helpers
}  // namespace routing
