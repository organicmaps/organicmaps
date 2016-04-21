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

/*!
 * \brief The LoadedPathSegment struct is a representation of a single node path.
 * It unpacks and stores information about path and road type flags.
 * Postprocessing must read information from the structure and does not initiate disk readings.
 */
struct LoadedPathSegment
{
  vector<m2::PointD> m_path;
  vector<turns::SingleLaneInfo> m_lanes;
  string m_name;
  TEdgeWeight m_weight;
  TNodeId m_nodeId;
  ftypes::HighwayClass m_highwayClass;
  bool m_onRoundabout;
  bool m_isLink;

  LoadedPathSegment()
  {
    Clear();
  }

  void Clear()
  {
    m_path.clear();
    m_lanes.clear();
    m_name.clear();
    m_weight = 0;
    m_nodeId = 0;
    m_highwayClass = ftypes::HighwayClass::Undefined;
    m_onRoundabout = false;
    m_isLink = false;
  }
};

using TUnpackedPathSegments = vector<LoadedPathSegment>;
}  // namespace routing
