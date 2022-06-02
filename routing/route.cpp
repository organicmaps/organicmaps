#include "routing/route.hpp"

#include "routing/turns_generator.hpp"

#include "traffic/speed_groups.hpp"

#include "indexer/feature_altitude.hpp"

#include "geometry/mercator.hpp"

#include "platform/location.hpp"

#include "geometry/angles.hpp"
#include "geometry/point2d.hpp"
#include "geometry/simplification.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <numeric>
#include <utility>

using namespace traffic;
using namespace routing::turns;
using namespace std;

namespace routing
{
namespace
{
double constexpr kOnEndToleranceM = 10.0;
double constexpr kSteetNameLinkMeters = 400.;
}  //  namespace

Route::Route(string const & router, vector<m2::PointD> const & points, uint64_t routeId,
             string const & name)
  : m_router(router)
  , m_routingSettings(GetRoutingSettings(VehicleType::Car))
  , m_name(name)
  , m_poly(points.begin(), points.end())
  , m_routeId(routeId)
{
}

void Route::AddAbsentCountry(string const & name)
{
  if (!name.empty())
    m_absentCountries.insert(name);
}

double Route::GetTotalDistanceMeters() const
{
  if (!m_poly.IsValid())
    return 0.0;
  return m_poly.GetTotalDistanceMeters();
}

double Route::GetCurrentDistanceFromBeginMeters() const
{
  if (!m_poly.IsValid())
    return 0.0;
  return m_poly.GetDistanceFromStartMeters();
}

double Route::GetCurrentDistanceToEndMeters() const
{
  if (!m_poly.IsValid())
    return 0.0;
  return m_poly.GetDistanceToEndMeters();
}

double Route::GetMercatorDistanceFromBegin() const
{
  auto const & curIter = m_poly.GetCurrentIter();
  if (!IsValid() || !curIter.IsValid())
    return 0;

  CHECK_LESS(curIter.m_ind, m_routeSegments.size(), ());

  double const distMerc =
      curIter.m_ind == 0 ? 0.0 : m_routeSegments[curIter.m_ind - 1].GetDistFromBeginningMerc();
  return distMerc + m_poly.GetDistFromCurPointToRoutePointMerc();
}

double Route::GetTotalTimeSec() const
{
  return m_routeSegments.empty() ? 0 : m_routeSegments.back().GetTimeFromBeginningSec();
}

double Route::GetCurrentTimeToEndSec() const
{
  auto const & curIter = m_poly.GetCurrentIter();
  if (!IsValid() || !curIter.IsValid())
    return 0.0;

  CHECK_LESS(curIter.m_ind, m_routeSegments.size(), ());
  double const etaToLastPassedPointS = GetETAToLastPassedPointSec();
  double const curSegLenMeters = GetSegLenMeters(curIter.m_ind);
  double const totalTimeS = GetTotalTimeSec();
  double const fromLastPassedPointToEndSec = totalTimeS - etaToLastPassedPointS;
  // Note. If a segment is short it does not make any sense to take into account time needed
  // to path its part.
  if (base::AlmostEqualAbs(curSegLenMeters, 0.0, 1.0 /* meters */))
    return fromLastPassedPointToEndSec;

  CHECK_LESS(curIter.m_ind, m_routeSegments.size(), ());
  // Pure fake edges should not be taken into account while ETA calculation.
  if (!m_routeSegments[curIter.m_ind].GetSegment().IsRealSegment())
    return fromLastPassedPointToEndSec;

  double const curSegTimeS = GetTimeToPassSegSec(curIter.m_ind);
  CHECK_GREATER(curSegTimeS, 0, ("Route can't contain segments with infinite speed."));

  double const curSegSpeedMPerS = curSegLenMeters / curSegTimeS;
  CHECK_GREATER(curSegSpeedMPerS, 0, ("Route can't contain segments with zero speed."));
  return totalTimeS - (etaToLastPassedPointS +
                       m_poly.GetDistFromCurPointToRoutePointMeters() / curSegSpeedMPerS);
}

void Route::GetCurrentStreetName(RouteSegment::RoadNameInfo & roadNameInfo) const
{
  GetStreetNameAfterIdx(static_cast<uint32_t>(m_poly.GetCurrentIter().m_ind), roadNameInfo);
}

void Route::GetNextTurnStreetName(RouteSegment::RoadNameInfo & roadNameInfo) const
{
  double distance;
  TurnItem turn;
  GetCurrentTurn(distance, turn);
  GetStreetNameAfterIdx(turn.m_index, roadNameInfo);
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
void Route::GetStreetNameAfterIdx(uint32_t idx, RouteSegment::RoadNameInfo & roadNameInfo) const
{
  auto const iterIdx = m_poly.GetIterToIndex(idx);
  if (!IsValid() || !iterIdx.IsValid())
    return;

  // Info about 1st segment with existing basic (non-link) info after link.
  RouteSegment::RoadNameInfo roadNameInfoNext;

  for (size_t i = idx; i < m_poly.GetPolyline().GetSize(); ++i)
  {
    // Note. curIter.m_ind == 0 means route iter at zero point. No corresponding route segments at
    // |m_routeSegments| in this case. |name| should be cleared.
    if (i == 0)
      continue;

    auto const & r = m_routeSegments[ConvertPointIdxToSegmentIdx(i)].GetRoadNameInfo();

    if (r.HasBasicTextInfo())
    {
      if (roadNameInfo.HasExitInfo())
        roadNameInfoNext = r;
      else
        roadNameInfo = r;
      break;
    }
    else if (r.HasExitInfo() && !roadNameInfo.HasExitInfo())
      roadNameInfo = r;
    // For exit wait for non-exit.
    else if (roadNameInfo.HasExitInfo() && !r.m_isLink)
      continue;
    // For non-exits check only during first |kSteetNameLinkMeters|.
    auto const furtherIter = m_poly.GetIterToIndex(i);
    CHECK(furtherIter.IsValid(), ());
    if (m_poly.GetDistanceM(iterIdx, furtherIter) > kSteetNameLinkMeters)
      break;
  }

  if (roadNameInfo.HasExitInfo())
  {
    // Use basic info from |roadNameInfoNext| to update |roadNameInfo|.
    if (roadNameInfo.m_destination_ref.empty())
      roadNameInfo.m_destination_ref = roadNameInfoNext.m_ref;
    roadNameInfo.m_name = roadNameInfoNext.m_name;
  }
}

size_t Route::ConvertPointIdxToSegmentIdx(size_t pointIdx) const
{
  CHECK_GREATER(pointIdx, 0, ());
  // Note. |pointIdx| is an index at |m_poly|. Properties of the point gets a segment at |m_routeSegments|
  // which precedes the point. So to get segment index it's needed to subtract one.
  CHECK_LESS(pointIdx, m_routeSegments.size() + 1, ());
  return pointIdx - 1;
}

void Route::GetClosestTurn(size_t segIdx, TurnItem & turn) const
{
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

void Route::GetCurrentTurn(double & distanceToTurnMeters, TurnItem & turn) const
{
  // Note. |m_poly.GetCurrentIter().m_ind| is a point index of last passed point at |m_poly|.
  GetClosestTurn(m_poly.GetCurrentIter().m_ind, turn);
  distanceToTurnMeters = m_poly.GetDistanceM(m_poly.GetCurrentIter(),
                                             m_poly.GetIterToIndex(turn.m_index));
}

optional<turns::TurnItem> Route::GetCurrentIteratorTurn() const
{
  auto const & iter = m_poly.GetCurrentIter();
  if (!iter.IsValid())
    return nullopt;

  CHECK_LESS(iter.m_ind, m_routeSegments.size(), ());
  return m_routeSegments[iter.m_ind].GetTurn();
}

bool Route::GetNextTurn(double & distanceToTurnMeters, TurnItem & nextTurn) const
{
  TurnItem curTurn;
  // Note. |m_poly.GetCurrentIter().m_ind| is a zero based index of last passed point at |m_poly|.
  size_t const curIdx = m_poly.GetCurrentIter().m_ind;
  // Note. First param of GetClosestTurn() is a segment index at |m_routeSegments|.
  // |curIdx| is an index of last passed point at |m_poly|.
  // |curIdx| + 1 is an index of next point.
  // |curIdx| + 1 - 1 is an index of segment to start look for the closest turn.
  GetClosestTurn(curIdx, curTurn);
  CHECK_LESS(curIdx, curTurn.m_index, ());
  if (curTurn.IsTurnReachedYourDestination())
  {
    nextTurn = TurnItem();
    return false;
  }

  // Note. |curTurn.m_index| is an index of the point of |curTurn| at polyline |m_poly|.
  // |curTurn.m_index| + 1 is an index of the next point after |curTurn|.
  // |curTurn.m_index| + 1 - 1 is an index of the segment next to the |curTurn| segment.
  CHECK_LESS(curTurn.m_index, m_routeSegments.size(), ());
  GetClosestTurn(curTurn.m_index, nextTurn);
  CHECK_LESS(curTurn.m_index, nextTurn.m_index, ());
  distanceToTurnMeters = m_poly.GetDistanceM(m_poly.GetCurrentIter(),
                                             m_poly.GetIterToIndex(nextTurn.m_index));
  return true;
}

bool Route::GetNextTurns(vector<TurnItemDist> & turns) const
{
  TurnItemDist currentTurn;
  GetCurrentTurn(currentTurn.m_distMeters, currentTurn.m_turnItem);

  turns.clear();
  turns.emplace_back(move(currentTurn));

  TurnItemDist nextTurn;
  if (GetNextTurn(nextTurn.m_distMeters, nextTurn.m_turnItem))
    turns.emplace_back(move(nextTurn));
  return true;
}

void Route::GetCurrentDirectionPoint(m2::PointD & pt) const
{
  m_poly.GetCurrentDirectionPoint(pt, kOnEndToleranceM);
}

void Route::SetRouteSegments(vector<RouteSegment> && routeSegments)
{
  vector<size_t> fakeSegmentIndexes;
  m_routeSegments = move(routeSegments);
  m_haveAltitudes = true;
  for (size_t i = 0; i < m_routeSegments.size(); ++i)
  {
    if (m_haveAltitudes &&
        m_routeSegments[i].GetJunction().GetAltitude() == geometry::kInvalidAltitude)
    {
      m_haveAltitudes = false;
    }

    if (!m_routeSegments[i].GetSegment().IsRealSegment())
      fakeSegmentIndexes.push_back(i);
  }

  m_poly.SetFakeSegmentIndexes(move(fakeSegmentIndexes));
}

bool Route::MoveIterator(location::GpsInfo const & info)
{
  m2::RectD const rect = mercator::MetersToXY(
      info.m_longitude, info.m_latitude,
      max(m_routingSettings.m_matchingThresholdM, info.m_horizontalAccuracy));

  return m_poly.UpdateMatchingProjection(rect);
}

double Route::GetPolySegAngle(size_t ind) const
{
  size_t const polySz = m_poly.GetPolyline().GetSize();

  if (ind + 1 >= polySz)
  {
    ASSERT(false, ());
    return 0;
  }

  m2::PointD const p1 = m_poly.GetPolyline().GetPoint(ind);
  m2::PointD p2;
  size_t i = ind + 1;
  do
  {
    p2 = m_poly.GetPolyline().GetPoint(i);
  }
  while (m2::AlmostEqualULPs(p1, p2) && ++i < polySz);
  return (i == polySz) ? 0 : base::RadToDeg(ang::AngleTo(p1, p2));
}

bool Route::MatchLocationToRoute(location::GpsInfo & location,
                                 location::RouteMatchingInfo & routeMatchingInfo) const
{
  if (!m_poly.IsValid())
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

size_t Route::GetSubrouteCount() const { return m_subrouteAttrs.size(); }

void Route::GetSubrouteInfo(size_t subrouteIdx, vector<RouteSegment> & segments) const
{
  segments.clear();
  SubrouteAttrs const & attrs = GetSubrouteAttrs(subrouteIdx);

  CHECK_LESS_OR_EQUAL(attrs.GetEndSegmentIdx(), m_routeSegments.size(), ());

  for (size_t i = attrs.GetBeginSegmentIdx(); i < attrs.GetEndSegmentIdx(); ++i)
    segments.push_back(m_routeSegments[i]);
}

Route::SubrouteAttrs const & Route::GetSubrouteAttrs(size_t subrouteIdx) const
{
  CHECK(IsValid(), ());
  CHECK_LESS(subrouteIdx, m_subrouteAttrs.size(), ());
  return m_subrouteAttrs[subrouteIdx];
}

Route::SubrouteSettings const Route::GetSubrouteSettings(size_t segmentIdx) const
{
  CHECK_LESS(segmentIdx, GetSubrouteCount(), ());
  return SubrouteSettings(m_routingSettings, m_router, m_subrouteUid);
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
  double const finishToleranceM = segmentIdx == m_routeSegments.size() - 1
                                      ? m_routingSettings.m_finishToleranceM
                                      : kOnEndToleranceM;
  return lengthMeters - passedDistanceMeters < finishToleranceM;
}

void Route::SetSubrouteUid(size_t segmentIdx, SubrouteUid subrouteUid)
{
  CHECK_LESS(segmentIdx, GetSubrouteCount(), ());
  m_subrouteUid = subrouteUid;
}

void Route::GetAltitudes(geometry::Altitudes & altitudes) const
{
  altitudes.clear();

  CHECK(!m_subrouteAttrs.empty(), ());
  altitudes.push_back(m_subrouteAttrs.front().GetStart().GetAltitude());

  for (auto const & s : m_routeSegments)
    altitudes.push_back(s.GetJunction().GetAltitude());
}

traffic::SpeedGroup Route::GetTraffic(size_t segmentIdx) const
{
  CHECK_LESS(segmentIdx, m_routeSegments.size(), ());
  return m_routeSegments[segmentIdx].GetTraffic();
}

void Route::GetTurnsForTesting(vector<TurnItem> & turns) const
{
  turns.clear();
  for (auto const & s : m_routeSegments)
  {
    if (IsNormalTurn(s.GetTurn()))
      turns.push_back(s.GetTurn());
  }
}

double Route::GetTimeToPassSegSec(size_t segIdx) const
{
  CHECK_LESS(segIdx, m_routeSegments.size(), ());
  return m_routeSegments[segIdx].GetTimeFromBeginningSec() -
         (segIdx == 0 ? 0.0 : m_routeSegments[segIdx - 1].GetTimeFromBeginningSec());
}

double Route::GetSegLenMeters(size_t segIdx) const
{
  CHECK_LESS(segIdx, m_routeSegments.size(), ());
  return m_routeSegments[segIdx].GetDistFromBeginningMeters() -
         (segIdx == 0 ? 0.0 : m_routeSegments[segIdx - 1].GetDistFromBeginningMeters());
}

void Route::SetMwmsPartlyProhibitedForSpeedCams(vector<platform::CountryFile> && mwms)
{
  m_speedCamPartlyProhibitedMwms = move(mwms);
}

bool Route::CrossMwmsPartlyProhibitedForSpeedCams() const
{
  return !m_speedCamPartlyProhibitedMwms.empty();
}

vector<platform::CountryFile> const & Route::GetMwmsPartlyProhibitedForSpeedCams() const
{
  return m_speedCamPartlyProhibitedMwms;
}

double Route::GetETAToLastPassedPointSec() const
{
  CHECK(IsValid(), ());
  auto const & curIter = m_poly.GetCurrentIter();
  CHECK(curIter.IsValid(), ());
  CHECK_LESS(curIter.m_ind, m_routeSegments.size(), ());

  return curIter.m_ind == 0 ? 0.0 : m_routeSegments[curIter.m_ind - 1].GetTimeFromBeginningSec();
}

bool IsNormalTurn(TurnItem const & turn)
{
  CHECK_NOT_EQUAL(turn.m_turn, CarDirection::Count, ());
  CHECK_NOT_EQUAL(turn.m_pedestrianTurn, PedestrianDirection::Count, ());

  return !turn.IsTurnNone();
}

string DebugPrint(Route const & r)
{
  return DebugPrint(r.m_poly.GetPolyline());
}
} // namespace routing
