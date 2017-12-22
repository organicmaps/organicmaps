#pragma once

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
  double m_weight = 0.0; /*!< Time in seconds to pass the segment. */
  SegmentRange m_segmentRange;
  vector<Segment> m_segments; /*!< Traffic segments for |m_path|. */
  ftypes::HighwayClass m_highwayClass = ftypes::HighwayClass::Undefined;
  bool m_onRoundabout = false;
  bool m_isLink = false;

  bool IsValid() const { return !m_path.empty(); }
};

using TUnpackedPathSegments = vector<LoadedPathSegment>;
}  // namespace routing
