#pragma once

#include "routing/osrm_helpers.hpp"
#include "routing/turns.hpp"
#include "routing/turn_candidate.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "base/buffer_vector.hpp"

#include "std/vector.hpp"

class Index;

namespace routing
{
struct RoutingMapping;
struct RawPathData;
struct FeatureGraphNode;

namespace turns
{
using TSeg = OsrmMappingTypes::FtSeg;

/*!
 * \brief The LoadedPathSegment struct is a representation of a single node path.
 * It unpacks and stores information about path and road type flags.
 * Postprocessing must read information from the structure and does not initiate disk readings.
 */
struct LoadedPathSegment
{
  vector<m2::PointD> m_path;
  ftypes::HighwayClass m_highwayClass;
  bool m_onRoundabout;
  bool m_isLink;
  TUniversalEdgeWeight m_weight;
  string m_name;
  TUniversalNodeId m_nodeId;
  vector<SingleLaneInfo> m_lanes;

  // General constructor.
  LoadedPathSegment(RoutingMapping & mapping, Index const & index,
                    RawPathData const & osrmPathSegment);
  // Special constructor for side nodes. Splits OSRM node by information from the FeatureGraphNode.
  LoadedPathSegment(RoutingMapping & mapping, Index const & index,
                    RawPathData const & osrmPathSegment, FeatureGraphNode const & startGraphNode,
                    FeatureGraphNode const & endGraphNode, bool isStartNode, bool isEndNode);

private:
  // Load information about road, that described as the sequence of FtSegs and start/end indexes in
  // in it. For the side case, it has information about start/end graph nodes.
  void LoadPathGeometry(buffer_vector<TSeg, 8> const & buffer, size_t startIndex,
                        size_t endIndex, Index const & index, RoutingMapping & mapping,
                        FeatureGraphNode const & startGraphNode,
                        FeatureGraphNode const & endGraphNode, bool isStartNode, bool isEndNode);
};
}  // namespace routing
}  // namespace turns
