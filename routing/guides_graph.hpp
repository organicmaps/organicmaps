#pragma once

#include "routing/fake_ending.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/segment.hpp"

#include "routing/base/small_list.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace routing
{
// Segment of the track in the guides graph.
struct GuideSegmentData
{
  LatLonWithAltitude m_pointFirst;
  LatLonWithAltitude m_pointLast;
  double m_weight = 0.0;
};

// Custom comparison optimized for Guides segments: they all have the same |m_mwmId| and
// |m_forward| is always true.
struct GuideSegmentCompare
{
  bool operator()(Segment const & lhs, Segment const & rhs) const;
};

using GuideSegments = std::map<Segment, GuideSegmentData, GuideSegmentCompare>;
using IterOnTrack = GuideSegments::const_iterator;

// Graph used in IndexGraphStarter for building routes through the guides tracks.
class GuidesGraph
{
public:
  GuidesGraph() = default;
  explicit GuidesGraph(double maxSpeedMpS, NumMwmId mwmId);

  Segment AddTrack(std::vector<geometry::PointWithAltitude> const & guideTrack, size_t requiredSegmentIdx);

  FakeEnding MakeFakeEnding(Segment const & segment, m2::PointD const & point,
                            geometry::PointWithAltitude const & projection) const;

  using EdgeListT = SmallList<SegmentEdge>;
  void GetEdgeList(Segment const & segment, bool isOutgoing, EdgeListT & edges, RouteWeight const & prevWeight) const;
  routing::LatLonWithAltitude const & GetJunction(Segment const & segment, bool front) const;
  RouteWeight CalcSegmentWeight(Segment const & segment) const;
  Segment FindSegment(Segment const & segment, size_t segmentIdx) const;

  // Returns back and front points of the segment. Segment belongs to one of the guides tracks.
  std::pair<LatLonWithAltitude, LatLonWithAltitude> GetFromTo(Segment const & segment) const;

  NumMwmId GetMwmId() const;

private:
  double CalcSegmentTimeS(LatLonWithAltitude const & point1, LatLonWithAltitude const & point2) const;

  GuideSegments m_segments;
  double m_maxSpeedMpS = 0.0;
  NumMwmId m_mwmId = 0;
  uint32_t m_curGuidesFeatrueId = 0;
};
}  // namespace routing
