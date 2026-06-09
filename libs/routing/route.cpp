#include "routing/route.hpp"
#include "routing/road_access.hpp"

#include "traffic/speed_groups.hpp"

#include "geometry/mercator.hpp"

#include "platform/location.hpp"

#include "geometry/angles.hpp"
#include "geometry/point2d.hpp"

#include <algorithm>
#include <set>
#include <tuple>
#include <utility>

namespace routing
{
using namespace routing::turns;
namespace
{
double constexpr kOnEndToleranceM = 10.0;
double constexpr kSteetNameLinkMeters = 400.0;
}  //  namespace

std::string DebugPrint(RouteSegment::RoadNameInfo const & rni)
{
  std::stringstream out;
  out << "RoadNameInfo "
      << "{ m_name = " << rni.m_name << ", m_ref = " << rni.m_ref << ", m_junction_ref = " << rni.m_junction_ref
      << ", m_destination_ref = " << rni.m_destination_ref << ", m_destination = " << rni.m_destination
      << ", m_isLink = " << rni.m_isLink << " }";
  return out.str();
}

std::string DebugPrint(RouteSegment::SpeedCamera const & rhs)
{
  return "SpeedCamera{ " + std::to_string(rhs.m_coef) + ", " + std::to_string(int(rhs.m_maxSpeedKmPH)) + " }";
}

////////////////////////////////////////////////////////////////////////////////////////
// RouteSegment implementation

bool RouteSegment::IsSegregatedTurn(RouteSegment const & from) const
{
  // Treat 'from' -> this turn is segregated candidate when (distance is checked by a caller):
  // - "st A" -> "st A" or
  // - Any -> unnamed link

  if (!m_roadNameInfo.m_name.empty())
    return m_roadNameInfo.m_name == from.m_roadNameInfo.m_name;
  else
    return m_roadNameInfo.m_isLink;
}

void RouteSegment::MergeLanes(RouteSegment & from)
{
  /// @todo Keep 'from' lanes now. Probably, can move and clear.
  if (m_turn.m_lanes.empty())
    m_turn.m_lanes = from.m_turn.m_lanes;
}

////////////////////////////////////////////////////////////////////////////////////////
// RouteBase implementation

void RouteBase::SetRouteSegments(std::vector<RouteSegment> && routeSegments)
{
  m_routeSegments = std::move(routeSegments);
  m_haveAltitudes = true;
  for (auto const & s : m_routeSegments)
  {
    if (s.GetJunction().GetAltitude() == geometry::kInvalidAltitude)
    {
      m_haveAltitudes = false;
      break;
    }
  }
}

double RouteBase::GetTotalTimeSec() const
{
  return m_routeSegments.empty() ? 0.0 : m_routeSegments.back().GetTimeFromBeginningSec();
}

m2::PointD RouteBase::GetMidpoint(size_t beginIdx, size_t endIdx) const
{
  ASSERT(IsValid(), ());
  ASSERT(beginIdx <= endIdx && endIdx < m_routeSegments.size(), (beginIdx, endIdx));

  double const startM = beginIdx == 0 ? 0.0 : m_routeSegments[beginIdx - 1].GetDistFromBeginningMeters();
  double const endM = m_routeSegments[endIdx].GetDistFromBeginningMeters();
  double const midM = startM + 0.5 * (endM - startM);
  for (size_t i = beginIdx; i <= endIdx; ++i)
  {
    double const e = m_routeSegments[i].GetDistFromBeginningMeters();
    if (e < midM)
      continue;
    double const s = i == 0 ? 0.0 : m_routeSegments[i - 1].GetDistFromBeginningMeters();
    double const t = (e > s) ? (midM - s) / (e - s) : 0.0;
    auto const & a =
        i == 0 ? m_subrouteAttrs.front().GetStart().GetPoint() : m_routeSegments[i - 1].GetJunction().GetPoint();
    auto const & b = m_routeSegments[i].GetJunction().GetPoint();
    return a + (b - a) * t;
  }
  return m_routeSegments[endIdx].GetJunction().GetPoint();
}

std::optional<m2::PointD> RouteBase::FindMaxDiffMidpoint(std::vector<RouteSegment> const & origin) const
{
  ASSERT(IsValid(), ());
  // Treat this route as a meaningful alternative only when the *total* geodesic length of segments
  // it doesn't share with |origin| is at least this fraction of its own total length. Below the
  // threshold the routes overlap enough that the user wouldn't tell them apart on the map.
  double constexpr kMinAltDiffFraction = 0.05;

  double const totalLenM = m_routeSegments.back().GetDistFromBeginningMeters();

  // (mwmId, featureId, segmentIdx) tuples of the origin route, direction-normalized so the same
  // edge traversed forward vs reverse is treated as identical. Fake start/end segments are skipped
  // on both sides — they're not real road features and would otherwise produce spurious "diff" hits.
  std::set<std::tuple<NumMwmId, uint32_t, uint32_t>> originFeatures;
  auto const key = [](RouteSegment const & s)
  {
    auto const & seg = s.GetSegment();
    return std::make_tuple(seg.GetMwmId(), seg.GetFeatureId(), seg.GetSegmentIdx());
  };
  for (auto const & s : origin)
    if (s.GetSegment().IsRealSegment())
      originFeatures.insert(key(s));

  // Single walk that tracks (a) total diff length — the significance signal that decides whether
  // this is really an alternative — and (b) the longest contiguous diff run, whose midpoint is
  // returned as the balloon pivot.
  double totalDiffLen = 0.0;
  double bestLen = 0.0;
  size_t bestRunStart = 0;
  size_t bestRunEnd = 0;
  bool haveBest = false;

  size_t runStartIdx = 0;
  bool inRun = false;

  auto closeRun = [&](size_t endIdx)
  {
    double const startM = runStartIdx == 0 ? 0.0 : m_routeSegments[runStartIdx - 1].GetDistFromBeginningMeters();
    double const endM = m_routeSegments[endIdx].GetDistFromBeginningMeters();
    double const len = endM - startM;
    totalDiffLen += len;
    if (len > bestLen)
    {
      bestLen = len;
      bestRunStart = runStartIdx;
      bestRunEnd = endIdx;
      haveBest = true;
    }
  };

  for (size_t i = 0; i < m_routeSegments.size(); ++i)
  {
    auto const & s = m_routeSegments[i];
    bool const diff = s.GetSegment().IsRealSegment() && !originFeatures.contains(key(s));
    if (diff)
    {
      if (!inRun)
      {
        inRun = true;
        runStartIdx = i;
      }
    }
    else if (inRun)
    {
      closeRun(i - 1);
      inRun = false;
    }
  }
  if (inRun)
    closeRun(m_routeSegments.size() - 1);

  if (!haveBest || totalDiffLen < kMinAltDiffFraction * totalLenM)
    return std::nullopt;

  return GetMidpoint(bestRunStart, bestRunEnd);
}

m2::RectD RouteBase::GetLimitRect() const
{
  m2::RectD rect;
  ForEachPoint([&rect](geometry::PointWithAltitude const & p) { rect.Add(p.GetPoint()); });
  return rect;
}

size_t RouteBase::GetSubrouteCount() const
{
  return m_subrouteAttrs.size();
}

void RouteBase::GetSubrouteInfo(size_t subrouteIdx, std::vector<RouteSegment> & segments) const
{
  segments.clear();
  SubrouteAttrs const & attrs = GetSubrouteAttrs(subrouteIdx);

  CHECK_LESS_OR_EQUAL(attrs.GetEndSegmentIdx(), m_routeSegments.size(), ());

  segments.reserve(attrs.GetSize());
  for (size_t i = attrs.GetBeginSegmentIdx(); i < attrs.GetEndSegmentIdx(); ++i)
    segments.push_back(m_routeSegments[i]);
}

RouteBase::SubrouteAttrs const & RouteBase::GetSubrouteAttrs(size_t subrouteIdx) const
{
  CHECK(IsValid(), ());
  CHECK_LESS(subrouteIdx, m_subrouteAttrs.size(), ());
  return m_subrouteAttrs[subrouteIdx];
}

void RouteBase::GetAltitudes(geometry::Altitudes & altitudes) const
{
  CHECK(!m_subrouteAttrs.empty(), ());

  altitudes.clear();
  altitudes.reserve(m_routeSegments.size() + 1);
  ForEachPoint([&altitudes](geometry::PointWithAltitude const & p) { altitudes.push_back(p.GetAltitude()); });
}

traffic::SpeedGroup RouteBase::GetTraffic(size_t segmentIdx) const
{
  CHECK_LESS(segmentIdx, m_routeSegments.size(), ());
  return m_routeSegments[segmentIdx].GetTraffic();
}

void RouteBase::GetTurnsForTesting(std::vector<TurnItem> & turns) const
{
  turns.clear();
  for (auto const & s : m_routeSegments)
    if (IsNormalTurn(s.GetTurn()))
      turns.push_back(s.GetTurn());
}

double RouteBase::GetSegLenMeters(size_t segIdx) const
{
  CHECK_LESS(segIdx, m_routeSegments.size(), ());
  return m_routeSegments[segIdx].GetDistFromBeginningMeters() -
         (segIdx == 0 ? 0.0 : m_routeSegments[segIdx - 1].GetDistFromBeginningMeters());
}

void RouteBase::SetMwmsPartlyProhibitedForSpeedCams(std::vector<platform::CountryFile> && mwms)
{
  m_speedCamPartlyProhibitedMwms = std::move(mwms);
}

bool RouteBase::CrossMwmsPartlyProhibitedForSpeedCams() const
{
  return !m_speedCamPartlyProhibitedMwms.empty();
}

std::vector<platform::CountryFile> const & RouteBase::GetMwmsPartlyProhibitedForSpeedCams() const
{
  return m_speedCamPartlyProhibitedMwms;
}

////////////////////////////////////////////////////////////////////////////////////////
// Route implementation (follow-state methods)

void Route::SetRouteSegments(std::vector<RouteSegment> && routeSegments)
{
  RouteBase::SetRouteSegments(std::move(routeSegments));
  UpdatePolyFakeIdx();
}

void Route::RebuildFollowedPolyline()
{
  // Reconstruct the followed polyline from base segments + first subroute start. Used when promoting
  // an alternative (RouteBase) to a followed Route.
  if (m_routeSegments.empty() || m_subrouteAttrs.empty())
  {
    FollowedPolyline().Swap(m_poly);
    return;
  }

  std::vector<m2::PointD> pts;
  pts.reserve(m_routeSegments.size() + 1);
  ForEachPoint([&pts](geometry::PointWithAltitude const & p) { pts.push_back(p.GetPoint()); });

  FollowedPolyline(pts.begin(), pts.end()).Swap(m_poly);
  UpdatePolySubrouteIdx();
  UpdatePolyFakeIdx();
}

void Route::UpdatePolyFakeIdx()
{
  std::vector<size_t> fakeIdx;
  for (size_t i = 0; i < m_routeSegments.size(); ++i)
    if (!m_routeSegments[i].GetSegment().IsRealSegment())
      fakeIdx.push_back(i);
  m_poly.SetFakeSegmentIndexes(std::move(fakeIdx));
}

double Route::GetCurrentDistanceFromBeginMeters() const
{
  ASSERT(IsValid(), ());
  return m_poly.GetDistanceFromStartMeters();
}

double Route::GetCurrentDistanceToEndMeters() const
{
  ASSERT(IsValid(), ());
  return m_poly.GetDistanceToEndMeters();
}

double Route::GetMercatorDistanceFromBegin() const
{
  ASSERT(IsValid(), ());

  auto const & curIter = m_poly.GetCurrentIter();
  ASSERT_LESS(curIter.m_ind, m_routeSegments.size(), ());

  double const distMerc = curIter.m_ind == 0 ? 0.0 : m_routeSegments[curIter.m_ind - 1].GetDistFromBeginningMerc();
  return distMerc + m_poly.GetDistFromCurPointToRoutePointMerc();
}

double Route::GetCurrentTimeToEndSec() const
{
  ASSERT(!m_routeSegments.empty(), ());
  return GetCurrentTimeToSegmentSec(m_routeSegments.size() - 1);
}

double Route::GetCurrentTimeToNearestTurnSec() const
{
  double distance;
  TurnItem turn;
  GetNearestTurn(distance, turn);

  // |turn.m_index| - 1 is the index of |turn| segment.
  CHECK_LESS_OR_EQUAL(turn.m_index, m_routeSegments.size(), ());
  CHECK_GREATER(turn.m_index, 0, ());

  return GetCurrentTimeToSegmentSec(turn.m_index - 1);
}

//           |     curSegLenMeters          |
//           |                              |
//           | passedSegMeters |            |
// ----------*-----------------*------------*----> x
//           |                 |            |
//           LastPoint     CurrentPoint     NextPoint
//
//           | fromLastPassedPointToEndSec
// ----------*-----------------*------------*----> t
//           |                 |            |
//           toLastPointSec    currentTime  toNextPointSec
//
// CurrentTime is calculated using equal proportions for distance and time at any segment.
double Route::GetCurrentTimeFromBeginSec() const
{
  ASSERT(IsValid(), ());

  auto const & curIter = m_poly.GetCurrentIter();
  CHECK_LESS(curIter.m_ind, m_routeSegments.size(), ());

  double const toLastPointSec =
      (curIter.m_ind == 0) ? 0.0 : m_routeSegments[curIter.m_ind - 1].GetTimeFromBeginningSec();
  double const toNextPointSec = m_routeSegments[curIter.m_ind].GetTimeFromBeginningSec();

  double const curSegLenMeters = GetSegLenMeters(curIter.m_ind);

  if (curSegLenMeters < 1.0)
    return toLastPointSec;

  double const passedSegMeters = m_poly.GetDistFromCurPointToRoutePointMeters();

  return toLastPointSec + passedSegMeters / curSegLenMeters * (toNextPointSec - toLastPointSec);
}

double Route::GetCurrentTimeToSegmentSec(size_t segIdx) const
{
  ASSERT(IsValid(), ());

  double const endTimeSec = m_routeSegments[segIdx].GetTimeFromBeginningSec();
  double const passedTimeSec = GetCurrentTimeFromBeginSec();

  return endTimeSec - passedTimeSec;
}

SpeedInUnits Route::GetCurrentSpeedLimit() const
{
  ASSERT(IsValid(), ());

  auto const idx = m_poly.GetCurrentIter().m_ind;
  if (idx < m_routeSegments.size())
    return m_routeSegments[idx].GetSpeedLimit(GetCurrentTimestamp());
  return {};
}

void Route::GetCurrentStreetName(RouteSegment::RoadNameInfo & roadNameInfo) const
{
  GetClosestStreetNameAfterIdx(m_poly.GetCurrentIter().m_ind, roadNameInfo);
}

void Route::GetNextTurnStreetName(RouteSegment::RoadNameInfo & roadNameInfo) const
{
  double distance;
  TurnItem turn;
  GetNearestTurn(distance, turn);
  GetClosestStreetNameAfterIdx(turn.m_index, roadNameInfo);
}

void Route::GetNextNextTurnStreetName(RouteSegment::RoadNameInfo & roadNameInfo) const
{
  double distance;
  TurnItem turn;
  GetNextTurn(distance, turn);
  GetClosestStreetNameAfterIdx(turn.m_index, roadNameInfo);
}

// If exit is false, returns ref and name.
// If exit is true, returns m_junction_ref, m_destination_ref, m_destination, m_name.
// Handling of incomplete data or non-standard data:
// - We go trough 400m to find first segment with existing data.
// But for link we go through all segments till we reach non-link segment,
// This is for really long links (e.g. 1km+ in USA) which have no tags.
// - Normally for link both destination and destination:ref tags exist together.
// But sometimes only |destination| tag exists for link. And |destination:ref| can be calculated
// by checking next segments of route until link will end and normal road will start.
// Hopefully it will have |ref| tag. So we can use it instead of |destination:ref|.
// Also we can use info about it's |name|. It can be useful if no |destination| tag.
// - Sometimes exit is not tagged as link (e.g. if new road starts here).
// At the same time they can have all useful tags just like link.
// Usually |destination:ref| = |ref| in such cases, or only 1st part of |destination:ref| can match.
void Route::GetClosestStreetNameAfterIdx(size_t segIdx, RouteSegment::RoadNameInfo & roadNameInfo) const
{
  roadNameInfo = {};

  if (!IsValid())
    return;

  // Info about 1st segment with existing basic (non-link) info after link.
  RouteSegment::RoadNameInfo roadNameInfoNext;

  // Note. m_current.m_ind == 0 means route iter at zero point.
  // No corresponding route segments at |m_routeSegments| in this case.
  for (size_t i = segIdx; i < m_routeSegments.size(); ++i)
  {
    auto const & r = m_routeSegments[i].GetRoadNameInfo();

    if (r.HasBasicTextInfo())
    {
      if (roadNameInfo.HasExitInfo())
        roadNameInfoNext = r;
      else
        roadNameInfo = r;
      break;
    }
    else if (r.HasExitTextInfo() || i == segIdx)
    {
      ASSERT(!roadNameInfo.HasBasicTextInfo(), ());
      if (!roadNameInfo.HasExitTextInfo())
        roadNameInfo = r;
    }

    // For exit wait for non-exit.
    if (roadNameInfo.HasExitInfo() && r.m_isLink)
      continue;

    // For non-exits check only during first |kSteetNameLinkMeters|.
    auto const startIter = m_poly.GetIterToIndex(segIdx);
    auto const furtherIter = m_poly.GetIterToIndex(i);
    if (m_poly.GetDistanceM(startIter, furtherIter) > kSteetNameLinkMeters)
      break;
  }

  if (roadNameInfo.HasExitInfo())
  {
    // Use basic info from |roadNameInfoNext| to update |roadNameInfo|.
    if (roadNameInfo.m_destination_ref.empty())
      roadNameInfo.m_destination_ref = roadNameInfoNext.m_ref;
    if (!roadNameInfoNext.m_name.empty())
      roadNameInfo.m_name = roadNameInfoNext.m_name;
  }
}

void Route::GetClosestTurnAfterIdx(size_t segIdx, TurnItem & turn) const
{
  if (!IsValid())
  {
    turn = {};
    return;
  }

  CHECK_LESS(segIdx, m_routeSegments.size(), ());

  for (size_t i = segIdx; i < m_routeSegments.size(); ++i)
  {
    if (IsNormalTurn(m_routeSegments[i].GetTurn()))
    {
      turn = m_routeSegments[i].GetTurn();
      return;
    }
  }
  CHECK(false, ("The last turn should be ReachedYourDestination."));
}

void Route::GetNearestTurn(double & distanceToTurnMeters, TurnItem & turn) const
{
  auto const & curIter = m_poly.GetCurrentIter();
  GetClosestTurnAfterIdx(curIter.m_ind, turn);
  CHECK_LESS(curIter.m_ind, turn.m_index, ());

  distanceToTurnMeters = m_poly.GetDistanceM(curIter, m_poly.GetIterToIndex(turn.m_index));
}

turns::TurnItem Route::GetCurrentIteratorTurn() const
{
  ASSERT(IsValid(), ());

  auto const & curIter = m_poly.GetCurrentIter();
  CHECK_LESS(curIter.m_ind, m_routeSegments.size(), ());
  return m_routeSegments[curIter.m_ind].GetTurn();
}

bool Route::GetNextTurn(double & distanceToTurnMeters, TurnItem & nextTurn) const
{
  TurnItem curTurn;
  auto const & curIter = m_poly.GetCurrentIter();
  size_t const curIdx = curIter.m_ind;
  GetClosestTurnAfterIdx(curIdx, curTurn);
  CHECK_LESS(curIdx, curTurn.m_index, ());
  if (curTurn.IsTurnReachedYourDestination())
  {
    nextTurn = TurnItem();
    return false;
  }

  CHECK_LESS(curTurn.m_index, m_routeSegments.size(), ());
  GetClosestTurnAfterIdx(curTurn.m_index, nextTurn);
  CHECK_LESS(curTurn.m_index, nextTurn.m_index, ());
  distanceToTurnMeters = m_poly.GetDistanceM(curIter, m_poly.GetIterToIndex(nextTurn.m_index));
  return true;
}

bool Route::GetNextTurns(std::vector<TurnItemDist> & turns) const
{
  TurnItemDist currentTurn;
  GetNearestTurn(currentTurn.m_distMeters, currentTurn.m_turnItem);

  turns.clear();
  turns.emplace_back(std::move(currentTurn));

  TurnItemDist nextTurn;
  if (GetNextTurn(nextTurn.m_distMeters, nextTurn.m_turnItem))
    turns.emplace_back(std::move(nextTurn));
  return true;
}

void Route::GetCurrentDirectionPoint(m2::PointD & pt) const
{
  m_poly.GetCurrentDirectionPoint(pt, kOnEndToleranceM);
}

bool Route::MoveIterator(location::GpsInfo const & info)
{
  m2::RectD const rect = mercator::MetersToXY(
      info.m_longitude, info.m_latitude, std::max(m_routingSettings.m_matchingThresholdM, info.m_horizontalAccuracy));

  return m_poly.UpdateMatchingProjection(rect);
}

double Route::GetPolySegAngle(size_t ind) const
{
  auto const & poly = m_poly.GetPolyline();
  size_t const polySz = poly.GetSize();

  if (ind + 1 >= polySz)
  {
    ASSERT(false, ());
    return 0;
  }

  m2::PointD const p1 = poly.GetPoint(ind);
  m2::PointD p2;
  size_t i = ind + 1;
  do
  {
    p2 = poly.GetPoint(i);
  }
  while (AlmostEqualULPs(p1, p2) && ++i < polySz);
  return (i == polySz) ? 0 : math::RadToDeg(ang::AngleTo(p1, p2));
}

bool Route::MatchLocationToRoute(location::GpsInfo & location, location::RouteMatchingInfo & routeMatchingInfo) const
{
  if (!IsValid())
    return false;

  auto const & iter = m_poly.GetCurrentIter();
  routeMatchingInfo.Set(iter.m_pt, iter.m_ind, GetMercatorDistanceFromBegin());

  auto const locationMerc = mercator::FromLatLon(location.m_latitude, location.m_longitude);
  auto const distFromRouteM = mercator::DistanceOnEarth(iter.m_pt, locationMerc);

  if (distFromRouteM < m_routingSettings.m_matchingThresholdM)
  {
    if (!m_poly.IsFakeSegment(iter.m_ind))
    {
      location.m_latitude = mercator::YToLat(iter.m_pt.y);
      location.m_longitude = mercator::XToLon(iter.m_pt.x);
      if (m_routingSettings.m_matchRoute)
        location.m_bearing = location::AngleToBearing(GetPolySegAngle(iter.m_ind));
      return true;
    }
  }
  return false;
}

bool Route::IsSubroutePassed(size_t subrouteIdx) const
{
  size_t const endSegmentIdx = GetSubrouteAttrs(subrouteIdx).GetEndSegmentIdx();
  // If all subroutes up to subrouteIdx are empty.
  if (endSegmentIdx == 0)
    return true;

  size_t const segmentIdx = endSegmentIdx - 1;
  CHECK_LESS(segmentIdx, m_routeSegments.size(), ());
  double const lengthMeters = m_routeSegments[segmentIdx].GetDistFromBeginningMeters();
  double const passedDistanceMeters = m_poly.GetDistanceFromStartMeters();
  double const finishToleranceM =
      segmentIdx == m_routeSegments.size() - 1 ? m_routingSettings.m_finishToleranceM : kOnEndToleranceM;
  return lengthMeters - passedDistanceMeters < finishToleranceM;
}

std::string Route::DebugPrintTurns() const
{
  std::string res;

  for (size_t i = 0; i < m_routeSegments.size(); ++i)
  {
    auto const & turn = m_routeSegments[i].GetTurn();

    // Always print first element as Start.
    if (i == 0 || !turn.IsTurnNone())
    {
      res += DebugPrint(mercator::ToLatLon(m_routeSegments[i].GetJunction()));
      res += "\n";

      res += DebugPrint(turn);
      res += "\n";

      RouteSegment::RoadNameInfo rni;
      GetClosestStreetNameAfterIdx(turn.m_index, rni);
      res += DebugPrint(rni);
      res += "\n";
    }
  }

  return res;
}

bool IsNormalTurn(TurnItem const & turn)
{
  CHECK_NOT_EQUAL(turn.m_turn, CarDirection::Count, ());
  CHECK_NOT_EQUAL(turn.m_pedestrianTurn, PedestrianDirection::Count, ());

  return !turn.IsTurnNone();
}

std::string DebugPrint(Route const & r)
{
  return DebugPrint(r.GetPoly());
}
}  // namespace routing
