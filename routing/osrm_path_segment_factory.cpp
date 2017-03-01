#include "routing/osrm_path_segment_factory.hpp"

#include "routing/road_graph.hpp"
#include "routing/routing_mapping.hpp"

#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

#include "base/buffer_vector.hpp"

namespace
{
// Osrm multiples seconds to 10, so we need to divide it back.
double constexpr kOSRMWeightToSecondsMultiplier = 1. / 10.;

using TSeg = routing::OsrmMappingTypes::FtSeg;

void LoadPathGeometry(buffer_vector<TSeg, 8> const & buffer, size_t startIndex,
                      size_t endIndex, Index const & index, routing::RoutingMapping & mapping,
                      routing::FeatureGraphNode const & startGraphNode,
                      routing::FeatureGraphNode const & endGraphNode, bool isStartNode,
                      bool isEndNode, routing::LoadedPathSegment & loadPathGeometry)
{
  ASSERT_LESS(startIndex, endIndex, ());
  ASSERT_LESS_OR_EQUAL(endIndex, buffer.size(), ());
  ASSERT(!buffer.empty(), ());
  for (size_t k = startIndex; k < endIndex; ++k)
  {
    auto const & segment = buffer[k];
    if (!segment.IsValid())
    {
      loadPathGeometry.m_path.clear();
      return;
    }

    // Load data from drive.
    FeatureType ft;
    Index::FeaturesLoaderGuard loader(index, mapping.GetMwmId());
    if (!loader.GetFeatureByIndex(segment.m_fid, ft))
    {
      loadPathGeometry.m_path.clear();
      return;
    }

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    // Get points in proper direction.
    auto startIdx = segment.m_pointStart;
    auto endIdx = segment.m_pointEnd;
    if (isStartNode && k == startIndex && startGraphNode.segment.IsValid())
      startIdx = (segment.m_pointEnd > segment.m_pointStart) ? startGraphNode.segment.m_pointStart
                                                             : startGraphNode.segment.m_pointEnd;
    if (isEndNode && k == endIndex - 1 && endGraphNode.segment.IsValid())
      endIdx = (segment.m_pointEnd > segment.m_pointStart) ? endGraphNode.segment.m_pointEnd
                                                           : endGraphNode.segment.m_pointStart;
    if (startIdx < endIdx)
    {
      for (auto idx = startIdx; idx <= endIdx; ++idx)
        loadPathGeometry.m_path.emplace_back(ft.GetPoint(idx), feature::kDefaultAltitudeMeters);
    }
    else
    {
      // I use big signed type because endIdx can be 0.
      for (int64_t idx = startIdx; idx >= static_cast<int64_t>(endIdx); --idx)
        loadPathGeometry.m_path.emplace_back(ft.GetPoint(idx), feature::kDefaultAltitudeMeters);
    }

    // Load lanes if it is a last segment before junction.
    if (buffer.back() == segment)
    {
      using feature::Metadata;
      Metadata const & md = ft.GetMetadata();

      auto directionType = Metadata::FMD_TURN_LANES;

      if (!ftypes::IsOneWayChecker::Instance()(ft))
      {
        directionType = (startIdx < endIdx) ? Metadata::FMD_TURN_LANES_FORWARD
                                            : Metadata::FMD_TURN_LANES_BACKWARD;
      }
      ParseLanes(md.Get(directionType), loadPathGeometry.m_lanes);
    }
    // Calculate node flags.
    loadPathGeometry.m_onRoundabout |= ftypes::IsRoundAboutChecker::Instance()(ft);
    loadPathGeometry.m_isLink |= ftypes::IsLinkChecker::Instance()(ft);
    loadPathGeometry.m_highwayClass = ftypes::GetHighwayClass(ft);
    string name;
    ft.GetName(FeatureType::DEFAULT_LANG, name);
    loadPathGeometry.m_name = name;
  }
}
}  // namespace

namespace routing
{
void OsrmPathSegmentFactory(RoutingMapping & mapping, Index const & index,
                            RawPathData const & osrmPathSegment, LoadedPathSegment & loadedPathSegment)
{
  loadedPathSegment.Clear();

  buffer_vector<TSeg, 8> buffer;
  mapping.m_segMapping.ForEachFtSeg(osrmPathSegment.node, MakeBackInsertFunctor(buffer));
  loadedPathSegment.m_weight = osrmPathSegment.segmentWeight * kOSRMWeightToSecondsMultiplier;
  loadedPathSegment.m_nodeId = UniNodeId(osrmPathSegment.node);
  if (buffer.empty())
  {
    LOG(LERROR, ("Can't unpack geometry for map:", mapping.GetCountryName(), " node: ",
                 osrmPathSegment.node));
    alohalytics::Stats::Instance().LogEvent(
        "RouteTracking_UnpackingError",
        {{"node", strings::to_string(osrmPathSegment.node)},
         {"map", mapping.GetCountryName()},
         {"version", strings::to_string(mapping.GetMwmId().GetInfo()->GetVersion())}});
    return;
  }
  LoadPathGeometry(buffer, 0, buffer.size(), index, mapping, FeatureGraphNode(), FeatureGraphNode(),
                   false /* isStartNode */, false /*isEndNode*/, loadedPathSegment);
}

void OsrmPathSegmentFactory(RoutingMapping & mapping, Index const & index, RawPathData const & osrmPathSegment,
                            FeatureGraphNode const & startGraphNode, FeatureGraphNode const & endGraphNode,
                            bool isStartNode, bool isEndNode, LoadedPathSegment & loadedPathSegment)
{
  ASSERT(isStartNode || isEndNode, ("This function process only corner cases."));
  loadedPathSegment.Clear();

  loadedPathSegment.m_nodeId = UniNodeId(osrmPathSegment.node);
  if (!startGraphNode.segment.IsValid() || !endGraphNode.segment.IsValid())
    return;
  buffer_vector<TSeg, 8> buffer;
  mapping.m_segMapping.ForEachFtSeg(osrmPathSegment.node, MakeBackInsertFunctor(buffer));

  auto findIntersectingSeg = [&buffer](TSeg const & seg) -> size_t
  {
    ASSERT(seg.IsValid(), ());
    auto const it = find_if(buffer.begin(), buffer.end(), [&seg](OsrmMappingTypes::FtSeg const & s)
                            {
                              return s.IsIntersect(seg);
                            });

    ASSERT(it != buffer.end(), ());
    return distance(buffer.begin(), it);
  };

  // Calculate estimated time for a start and a end node cases.
  if (isStartNode && isEndNode)
  {
    double const forwardWeight = (osrmPathSegment.node == startGraphNode.node.forward_node_id)
                                     ? startGraphNode.node.forward_weight
                                     : startGraphNode.node.reverse_weight;
    double const backwardWeight = (osrmPathSegment.node == endGraphNode.node.forward_node_id)
                                      ? endGraphNode.node.forward_weight
                                      : endGraphNode.node.reverse_weight;
    double const wholeWeight = (osrmPathSegment.node == startGraphNode.node.forward_node_id)
                                   ? startGraphNode.node.forward_offset
                                   : startGraphNode.node.reverse_offset;
    // Sum because weights in forward/backward_weight fields are negative. Look osrm_helpers for
    // more info.
    loadedPathSegment.m_weight = wholeWeight + forwardWeight + backwardWeight;
  }
  else
  {
    PhantomNode const * node = nullptr;
    if (isStartNode)
      node = &startGraphNode.node;
    if (isEndNode)
      node = &endGraphNode.node;
    if (node)
    {
      loadedPathSegment.m_weight = (osrmPathSegment.node == node->forward_weight)
          ? node->GetForwardWeightPlusOffset() : node->GetReverseWeightPlusOffset();
    }
  }

  size_t startIndex = isStartNode ? findIntersectingSeg(startGraphNode.segment) : 0;
  size_t endIndex = isEndNode ? findIntersectingSeg(endGraphNode.segment) + 1 : buffer.size();
  LoadPathGeometry(buffer, startIndex, endIndex, index, mapping, startGraphNode, endGraphNode, isStartNode,
                   isEndNode, loadedPathSegment);
  loadedPathSegment.m_weight *= kOSRMWeightToSecondsMultiplier;
}
}  // namespace routing
