#pragma once

#include "routing/road_graph.hpp"
#include "routing/road_point.hpp"
#include "routing/turns.hpp"
#include "routing/turn_candidate.hpp"
#include "routing/segment.hpp"

#include "traffic/traffic_info.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "geometry/point_with_altitude.hpp"

#include "base/buffer_vector.hpp"

#include <string>
#include <vector>

namespace routing
{
/*!
 * \brief The LoadedPathSegment struct is a representation of a single node path.
 * It unpacks and stores information about path and road type flags.
 * Postprocessing must read information from the structure and does not initiate disk readings.
 */
struct LoadedPathSegment
{
  std::vector<geometry::PointWithAltitude> m_path;
  std::vector<turns::SingleLaneInfo> m_lanes;
  std::string m_name;
  double m_weight = 0.0; /*!< Time in seconds to pass the segment. */
  SegmentRange m_segmentRange;
  std::vector<Segment> m_segments; /*!< Traffic segments for |m_path|. */
  ftypes::HighwayClass m_highwayClass = ftypes::HighwayClass::Undefined;
  bool m_onRoundabout = false;
  bool m_isLink = false;

  bool IsValid() const { return m_path.size() > 1; }
};

using TUnpackedPathSegments = std::vector<LoadedPathSegment>;
}  // namespace routing
