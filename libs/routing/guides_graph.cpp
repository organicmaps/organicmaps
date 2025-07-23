#include "routing/guides_graph.hpp"

#include "routing/fake_feature_ids.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include <algorithm>
#include <limits>
#include <tuple>

namespace
{
double constexpr kZeroAltitude = 0.0;
}  // namespace

namespace routing
{
bool GuideSegmentCompare::operator()(Segment const & lhs, Segment const & rhs) const
{
  if (lhs.GetFeatureId() != rhs.GetFeatureId())
    return lhs.GetFeatureId() < rhs.GetFeatureId();

  return lhs.GetSegmentIdx() < rhs.GetSegmentIdx();
}

GuidesGraph::GuidesGraph(double maxSpeedMpS, NumMwmId mwmId)
  : m_maxSpeedMpS(maxSpeedMpS)
  , m_mwmId(mwmId)
  , m_curGuidesFeatrueId(FakeFeatureIds::kGuidesGraphFeaturesStart)
{
  CHECK_NOT_EQUAL(m_maxSpeedMpS, 0.0, ());
}

double GuidesGraph::CalcSegmentTimeS(LatLonWithAltitude const & point1, LatLonWithAltitude const & point2) const
{
  double const distM = ms::DistanceOnEarth(point1.GetLatLon(), point2.GetLatLon());
  double const weight = distM / m_maxSpeedMpS;
  return weight;
}

void GuidesGraph::GetEdgeList(Segment const & segment, bool isOutgoing, EdgeListT & edges,
                              RouteWeight const & prevWeight) const
{
  auto const it = m_segments.find(segment);
  CHECK(it != m_segments.end(), (segment));
  auto const & segmentOnTrack = it->first;

  IterOnTrack itOnTrack;

  if (segment.IsForward() == isOutgoing)
  {
    Segment nextSegment(segmentOnTrack.GetMwmId(), segmentOnTrack.GetFeatureId(), segmentOnTrack.GetSegmentIdx() + 1,
                        segmentOnTrack.IsForward());
    itOnTrack = m_segments.find(nextSegment);
    if (itOnTrack == m_segments.end())
      return;
  }
  else
  {
    if (it->first.GetSegmentIdx() == 0)
      return;

    Segment prevSegment(segmentOnTrack.GetMwmId(), segmentOnTrack.GetFeatureId(), segmentOnTrack.GetSegmentIdx() - 1,
                        segmentOnTrack.IsForward());
    itOnTrack = m_segments.find(prevSegment);
    CHECK(itOnTrack != m_segments.end(), (segment));
  }
  auto const & neighbour = itOnTrack->first;
  Segment const resSegment(neighbour.GetMwmId(), neighbour.GetFeatureId(), neighbour.GetSegmentIdx(),
                           segment.IsForward());

  auto const weight = isOutgoing ? RouteWeight(itOnTrack->second.m_weight) : prevWeight;
  edges.emplace_back(resSegment, weight);
}

LatLonWithAltitude const & GuidesGraph::GetJunction(Segment const & segment, bool front) const
{
  auto const it = m_segments.find(segment);
  CHECK(it != m_segments.end(), (segment));
  return segment.IsForward() == front ? it->second.m_pointLast : it->second.m_pointFirst;
}

NumMwmId GuidesGraph::GetMwmId() const
{
  return m_mwmId;
}

Segment GuidesGraph::AddTrack(std::vector<geometry::PointWithAltitude> const & guideTrack, size_t requiredSegmentIdx)
{
  uint32_t segmentIdx = 0;
  Segment segment;

  for (size_t i = 0; i < guideTrack.size() - 1; ++i)
  {
    GuideSegmentData data;
    Segment curSegment(m_mwmId, m_curGuidesFeatrueId, segmentIdx++, true /* forward */);
    if (i == requiredSegmentIdx)
      segment = curSegment;

    data.m_pointFirst = LatLonWithAltitude(mercator::ToLatLon(guideTrack[i].GetPoint()), guideTrack[i].GetAltitude());
    data.m_pointLast =
        LatLonWithAltitude(mercator::ToLatLon(guideTrack[i + 1].GetPoint()), guideTrack[i + 1].GetAltitude());

    data.m_weight = CalcSegmentTimeS(data.m_pointFirst, data.m_pointLast);

    auto const [it, inserted] = m_segments.emplace(curSegment, data);
    CHECK(inserted, (curSegment));
  }
  ++m_curGuidesFeatrueId;
  return segment;
}

Segment GuidesGraph::FindSegment(Segment const & segment, size_t segmentIdx) const
{
  auto const it = m_segments.find(segment);
  CHECK(it != m_segments.end(), (segment, segmentIdx));
  Segment segmentOnTrack(it->first.GetMwmId(), it->first.GetFeatureId(), static_cast<uint32_t>(segmentIdx),
                         it->first.IsForward());

  auto const itByIndex = m_segments.find(segmentOnTrack);
  CHECK(itByIndex != m_segments.end(), (segment, segmentIdx));
  return itByIndex->first;
}

RouteWeight GuidesGraph::CalcSegmentWeight(Segment const & segment) const
{
  auto const it = m_segments.find(segment);
  CHECK(it != m_segments.end(), (segment));

  return RouteWeight(it->second.m_weight);
}

FakeEnding GuidesGraph::MakeFakeEnding(Segment const & segment, m2::PointD const & point,
                                       geometry::PointWithAltitude const & projection) const
{
  auto const & frontJunction = GetJunction(segment, true /* front */);
  auto const & backJunction = GetJunction(segment, false /* front */);

  FakeEnding ending;
  ending.m_projections.emplace_back(
      segment, false /* isOneWay */, frontJunction, backJunction,
      LatLonWithAltitude(mercator::ToLatLon(projection.GetPoint()), projection.GetAltitude()) /* junction */);

  ending.m_originJunction =
      LatLonWithAltitude(mercator::ToLatLon(point), static_cast<geometry::Altitude>(kZeroAltitude));
  return ending;
}

std::pair<LatLonWithAltitude, LatLonWithAltitude> GuidesGraph::GetFromTo(Segment const & segment) const
{
  auto const & from = GetJunction(segment, false /* front */);
  auto const & to = GetJunction(segment, true /* front */);
  return std::make_pair(from, to);
}
}  // namespace routing
