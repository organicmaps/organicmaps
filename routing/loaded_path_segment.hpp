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
  TEdgeWeight m_weight;
  string m_name;
  TNodeId m_nodeId;
  vector<SingleLaneInfo> m_lanes;

  LoadedPathSegment(TEdgeWeight weight, TNodeId nodeId)
    : m_highwayClass(ftypes::HighwayClass::Undefined)
    , m_onRoundabout(false)
    , m_isLink(false)
    , m_weight(weight)
    , m_nodeId(nodeId)
  {
  }
};
}  // namespace routing
}  // namespace turns
