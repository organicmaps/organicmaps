#pragma once

#include "routing/osrm_helpers.hpp"
#include "routing/road_graph.hpp"
#include "routing/road_point.hpp"
#include "routing/turns.hpp"
#include "routing/turn_candidate.hpp"
#include "routing/segment.hpp"

#include "traffic/traffic_info.hpp"

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
  vector<Junction> m_path;
  vector<turns::SingleLaneInfo> m_lanes;
  string m_name;
  TEdgeWeight m_weight; /*!< Time in seconds to pass the segment. */
  UniNodeId m_nodeId;   /*!< May be either NodeID for OSRM route or
                             mwm id, feature id, segment id and direction for A*. */
  vector<Segment> m_trafficSegs; /*!< Traffic segments for |m_path|. */
  ftypes::HighwayClass m_highwayClass;
  bool m_onRoundabout;
  bool m_isLink;

  LoadedPathSegment(UniNodeId::Type type) : m_nodeId(type) { Clear(); }
  void Clear()
  {
    m_path.clear();
    m_lanes.clear();
    m_name.clear();
    m_weight = 0;
    m_nodeId.Clear();
    m_trafficSegs.clear();
    m_highwayClass = ftypes::HighwayClass::Undefined;
    m_onRoundabout = false;
    m_isLink = false;
  }
};

using TUnpackedPathSegments = vector<LoadedPathSegment>;
}  // namespace routing
