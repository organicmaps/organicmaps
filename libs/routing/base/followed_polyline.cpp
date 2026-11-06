#include "followed_polyline.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/buffer_vector.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace routing
{
using Iter = routing::FollowedPolyline::Iter;
namespace
{
// Match equivalent projections within GPS uncertainty, with a sub-metre lower bound.
double constexpr kMinSpatialToleranceM = 0.5;
// Ignore small bearing differences when comparing source occurrences.
double constexpr kDirectionAlignmentTolerance = 0.1;
// Forward-only continuation applies to a near-straight source chain.
double constexpr kForwardChainMinCosine = 0.9;
// Allow bearing noise while rejecting perpendicular or opposing rejoin directions.
double constexpr kMinDirectionMatchCosine = 0.5;

struct TrackCandidate
{
  Iter m_iter;
  double m_distanceM = 0.0;
  double m_signedChangeM = 0.0;
  double m_alignment = 0.0;
  bool m_onForwardChain = false;
  bool m_afterObservedTurn = false;

  // Prefer continuity among projections in the same spatial uncertainty band.
  bool PreferTo(TrackCandidate const & best, double toleranceM) const
  {
    if (!best.m_iter.IsValid())
      return true;
    // Within one forward occurrence, exact proximity advances dense samples.
    if (m_onForwardChain && best.m_onForwardChain && m_signedChangeM > 0.0 && m_distanceM < best.m_distanceM)
      return true;
    double const change = fabs(m_signedChangeM);
    double const bestChange = fabs(best.m_signedChangeM);
    if (change < bestChange - toleranceM)
      return true;
    if (fabs(change - bestChange) > toleranceM)
      return false;
    if (m_alignment > best.m_alignment + kDirectionAlignmentTolerance)
      return true;
    if (fabs(m_alignment - best.m_alignment) > kDirectionAlignmentTolerance)
      return false;
    return (m_afterObservedTurn && best.m_signedChangeM < 0.0) || change < bestChange;
  }
};
}  // namespace

Iter FollowedPolyline::Begin() const
{
  ASSERT(IsValid(), ());
  return Iter(m_poly.Front(), 0);
}

Iter FollowedPolyline::End() const
{
  ASSERT(IsValid(), ());
  return Iter(m_poly.Back(), m_poly.GetSize() - 1);
}

Iter FollowedPolyline::GetIterToIndex(size_t index) const
{
  ASSERT(IsValid(), ());
  ASSERT_LESS(index, m_poly.GetSize(), ());

  return Iter(m_poly.GetPoint(index), index);
}

double FollowedPolyline::GetDistanceM(Iter const & it1, Iter const & it2) const
{
  ASSERT(IsValid(), ());
  ASSERT(it1.IsValid() && it2.IsValid(), ());
  ASSERT_LESS_OR_EQUAL(it1.m_ind, it2.m_ind, ());
  ASSERT_LESS(it1.m_ind, m_poly.GetSize(), ());
  ASSERT_LESS(it2.m_ind, m_poly.GetSize(), ());

  if (it1.m_ind == it2.m_ind)
    return mercator::DistanceOnEarth(it1.m_pt, it2.m_pt);

  return (mercator::DistanceOnEarth(it1.m_pt, m_poly.GetPoint(it1.m_ind + 1)) + m_segDistance[it2.m_ind - 1] -
          m_segDistance[it1.m_ind] + mercator::DistanceOnEarth(m_poly.GetPoint(it2.m_ind), it2.m_pt));
}

double FollowedPolyline::GetTotalDistanceMeters() const
{
  if (!IsValid())
  {
    ASSERT(IsValid(), ());
    return 0;
  }
  return m_segDistance.back();
}

double FollowedPolyline::GetDistanceFromStartMeters() const
{
  if (!IsValid() || !m_current.IsValid())
  {
    ASSERT(IsValid(), ());
    ASSERT(m_current.IsValid(), ());
    return 0;
  }

  return (m_current.m_ind > 0 ? m_segDistance[m_current.m_ind - 1] : 0.0) +
         mercator::DistanceOnEarth(m_current.m_pt, m_poly.GetPoint(m_current.m_ind));
}

double FollowedPolyline::GetDistanceToEndMeters() const
{
  return GetTotalDistanceMeters() - GetDistanceFromStartMeters();
}

void FollowedPolyline::Swap(FollowedPolyline & rhs)
{
  m_poly.Swap(rhs.m_poly);
  m_segDistance.swap(rhs.m_segDistance);
  m_segProj.swap(rhs.m_segProj);
  std::swap(m_current, rhs.m_current);
  std::swap(m_nextCheckpointIndex, rhs.m_nextCheckpointIndex);
  std::swap(m_lastNonZeroSegment, rhs.m_lastNonZeroSegment);
}

void FollowedPolyline::Update()
{
  size_t n = m_poly.GetSize();
  ASSERT_GREATER(n, 1, ());
  --n;

  m_segDistance.clear();
  m_segDistance.reserve(n);
  m_segProj.clear();
  m_segProj.reserve(n);

  double dist = 0.0;
  m_lastNonZeroSegment = 0;
  for (size_t i = 0; i < n; ++i)
  {
    m2::PointD const & p1 = m_poly.GetPoint(i);
    m2::PointD const & p2 = m_poly.GetPoint(i + 1);

    dist += mercator::DistanceOnEarth(p1, p2);
    if (!p1.EqualDxDy(p2, mercator::kPointEqualityEps))
      m_lastNonZeroSegment = i;

    m_segDistance.emplace_back(dist);
    m_segProj.emplace_back(p1, p2);
  }

  m_current = Iter(m_poly.Front(), 0);
}

bool FollowedPolyline::IsFakeSegment(size_t index) const
{
  return std::binary_search(m_fakeSegmentIndexes.begin(), m_fakeSegmentIndexes.end(), index);
}

template <class DistanceFn>
Iter FollowedPolyline::GetBestProjection(m2::RectD const & posRect, DistanceFn const & distFn) const
{
  CHECK_EQUAL(m_segProj.size() + 1, m_poly.GetSize(), ());
  // At first trying to find a projection to two closest route segments of route which is close
  // enough to |posRect| center. If |m_current| is right before intermediate point we can get |closestIter|
  // right after intermediate point (in next subroute).
  size_t const hoppingBorderIdx = std::min(m_segProj.size(), m_current.m_ind + 2);
  Iter const closestIter = GetClosestProjectionInInterval(posRect, distFn, m_current.m_ind, hoppingBorderIdx);
  if (closestIter.IsValid())
    return closestIter;

  // If a projection to the two closest route segments is not found tries to find projection to other route
  // segments of current subroute.
  return GetClosestProjectionInInterval(posRect, distFn, hoppingBorderIdx, m_nextCheckpointIndex);
}

Iter FollowedPolyline::GetBestMatchingProjection(m2::RectD const & posRect) const
{
  CHECK_EQUAL(m_segProj.size() + 1, m_poly.GetSize(), ());
  // At first trying to find a projection to two closest route segments of route which is close
  // enough to |posRect| center. If |m_current| is right before intermediate point we can get |iter|
  // right after intermediate point (in next subroute).
  size_t const hoppingBorderIdx = std::min(m_nextCheckpointIndex, std::min(m_segProj.size(), m_current.m_ind + 3));
  auto const iter = GetClosestMatchingProjectionInInterval(posRect, m_current.m_ind, hoppingBorderIdx);
  if (iter.IsValid())
    return iter;
  // If a projection to the 3 closest route segments is not found tries to find projection to other route
  // segments of current subroute.
  return GetClosestMatchingProjectionInInterval(posRect, hoppingBorderIdx, m_nextCheckpointIndex);
}

bool FollowedPolyline::UpdateMatchingProjection(m2::RectD const & posRect)
{
  ASSERT(m_current.IsValid(), ());
  ASSERT_LESS(m_current.m_ind, m_poly.GetSize() - 1, ());

  auto const iter = GetBestMatchingProjection(posRect);

  if (iter.IsValid())
  {
    m_current = iter;
    return true;
  }

  return false;
}

double FollowedPolyline::GetLastNonZeroSegmentStartMeters() const
{
  ASSERT(IsValid(), ());
  return m_lastNonZeroSegment == 0 ? 0.0 : m_segDistance[m_lastNonZeroSegment - 1];
}

size_t FollowedPolyline::FindNextNonZeroSegment(size_t segment) const
{
  while (segment < m_segProj.size() &&
         m_poly.GetPoint(segment).EqualDxDy(m_poly.GetPoint(segment + 1), mercator::kPointEqualityEps))
    ++segment;
  return segment;
}

size_t FollowedPolyline::GetForwardChainEnd(m2::PointD const & direction, double maxDistanceM, double toleranceM) const
{
  ASSERT_GREATER(direction.SquaredLength(), 0.0, ());
  size_t end = m_current.m_ind;
  auto const origin = m_current.m_pt;
  double const directionLength = direction.Length();
  double const tolerance = toleranceM * directionLength / mercator::DistanceOnEarth(origin, origin + direction);
  double furthest = 0.0;
  std::unordered_set<m2::PointD, m2::PointD::Hash> visited;
  visited.insert(origin);
  // Nearby earlier vertices distinguish source revisits from recording noise.
  for (size_t i = m_current.m_ind + 1; i > 0; --i)
  {
    auto const & point = m_poly.GetPoint(i - 1);
    if (mercator::DistanceOnEarth(origin, point) > toleranceM)
      break;
    visited.insert(point);
  }
  for (size_t i = m_current.m_ind; i < m_segProj.size(); ++i)
  {
    auto const & point = m_poly.GetPoint(i + 1);
    // Consecutive altitude/stop samples belong to one visit; returning to a vertex does not.
    bool const atCurrent = i == m_current.m_ind && point.EqualDxDy(origin, mercator::kPointEqualityEps);
    if (!point.EqualDxDy(m_poly.GetPoint(i), mercator::kPointEqualityEps) && !visited.insert(point).second && !atCurrent)
      break;
    auto const chord = point - origin;
    double const projection = m2::DotProduct(direction, chord) / directionLength;
    // Individual edges can backtrack within GPS uncertainty; sustained reversal ends this visit.
    if (projection < furthest - tolerance)
      break;
    furthest = std::max(furthest, projection);
    double const distanceM = mercator::DistanceOnEarth(origin, point);
    if (distanceM > 2.0 * toleranceM &&
        m2::DotProduct(direction, chord) < kForwardChainMinCosine * directionLength * chord.Length())
      break;
    end = i;
    if (distanceM > maxDistanceM)
      break;
  }
  return end;
}

bool FollowedPolyline::UpdateTrackMatchingProjection(TrackMatchQuery const & query)
{
  auto const iter = GetTrackMatchingProjection(query);
  if (!iter.IsValid())
    return false;
  m_current = iter;
  return true;
}

Iter FollowedPolyline::GetTrackMatchingProjection(TrackMatchQuery const & query) const
{
  ASSERT(m_current.IsValid(), ());
  ASSERT_LESS(m_current.m_ind, m_segProj.size(), ());
  ASSERT_GREATER_OR_EQUAL(query.m_minProgressM, 0.0, ());
  ASSERT_LESS_OR_EQUAL(query.m_minProgressM, query.m_maxProgressM, ());
  ASSERT_GREATER_OR_EQUAL(query.m_toleranceM, 0.0, ());
  auto const currentDistance = GetDistanceFromStartMeters();
  size_t const start =
      std::lower_bound(m_segDistance.begin(), m_segDistance.end(), query.m_minProgressM) - m_segDistance.begin();
  size_t const nextSegment = FindNextNonZeroSegment(m_current.m_ind + 1);
  bool const confirmedTurn =
      mercator::DistanceOnEarth(m_current.m_pt, m_poly.GetPoint(m_current.m_ind + 1)) <= query.m_toleranceM;
  double const forwardAllowanceM = std::max(0.0, query.m_maxProgressM - currentDistance);
  size_t const forwardEnd = query.m_forwardDirection
                              ? GetForwardChainEnd(*query.m_forwardDirection, forwardAllowanceM, query.m_toleranceM)
                              : m_current.m_ind;
  TrackCandidate best;
  buffer_vector<TrackCandidate, 32> candidates;
  double nearestDistanceM = std::numeric_limits<double>::max();
  for (size_t i = start; i < m_segProj.size(); ++i)
  {
    double const segmentStart = i == 0 ? 0.0 : m_segDistance[i - 1];
    if (segmentStart > query.m_maxProgressM && i > forwardEnd)
      break;
    auto const point = m_segProj[i].ClosestPointTo(query.m_positionRect.Center());
    if (!query.m_positionRect.IsPointInside(point))
      continue;
    auto const & from = m_poly.GetPoint(i);
    auto const delta = m_poly.GetPoint(i + 1) - from;
    double const progress = segmentStart + mercator::DistanceOnEarth(from, point);
    bool const onForwardChain = query.m_forwardDirection && progress >= currentDistance && i >= m_current.m_ind &&
                                i <= forwardEnd &&
                                mercator::DistanceOnEarth(m_current.m_pt, point) <= forwardAllowanceM;
    if (progress < query.m_minProgressM || (progress > query.m_maxProgressM && !onForwardChain))
      continue;
    double const change = progress - currentDistance;
    double const alignment =
        query.m_direction && delta.SquaredLength() > 0.0
            ? m2::DotProduct(delta, *query.m_direction) / delta.Length() * (change < 0.0 ? -1.0 : 1.0)
            : 0.0;
    if (query.m_requireDirectionMatch && alignment < kMinDirectionMatchCosine)
      continue;
    TrackCandidate const candidate{Iter(point, i),
                                   mercator::DistanceOnEarth(point, query.m_positionRect.Center()),
                                   change,
                                   alignment,
                                   onForwardChain && !query.m_isFirstFix,
                                   change > 0.0 && confirmedTurn && i <= nextSegment};
    nearestDistanceM = std::min(nearestDistanceM, candidate.m_distanceM);
    candidates.push_back(candidate);
  }
  // Anchor uncertainty to the nearest projection, so it cannot grow across dense samples.
  double const distanceLimitM = nearestDistanceM + std::max(kMinSpatialToleranceM, query.m_toleranceM);
  for (auto const & candidate : candidates)
    if (candidate.m_distanceM <= distanceLimitM && candidate.PreferTo(best, query.m_toleranceM))
      best = candidate;
  return best.m_iter;
}

Iter FollowedPolyline::UpdateProjection(m2::RectD const & posRect)
{
  ASSERT(m_current.IsValid(), ());
  ASSERT_LESS(m_current.m_ind, m_poly.GetSize() - 1, ());

  Iter res;
  m2::PointD const currPos = posRect.Center();
  res = GetBestProjection(posRect, [&](Iter const & it) { return mercator::DistanceOnEarth(it.m_pt, currPos); });

  if (res.IsValid())
    m_current = res;
  return res;
}

void FollowedPolyline::SetFakeSegmentIndexes(std::vector<size_t> && fakeSegmentIndexes)
{
  ASSERT(std::is_sorted(fakeSegmentIndexes.cbegin(), fakeSegmentIndexes.cend()), ());
  m_fakeSegmentIndexes = std::move(fakeSegmentIndexes);
}

double FollowedPolyline::GetDistFromCurPointToRoutePointMerc() const
{
  if (!m_current.IsValid())
    return 0.0;

  return m_poly.GetPoint(m_current.m_ind).Length(m_current.m_pt);
}

double FollowedPolyline::GetDistFromCurPointToRoutePointMeters() const
{
  if (!m_current.IsValid())
    return 0.0;

  return mercator::DistanceOnEarth(m_poly.GetPoint(m_current.m_ind), m_current.m_pt);
}

void FollowedPolyline::GetCurrentDirectionPoint(m2::PointD & pt, double toleranceM) const
{
  ASSERT(IsValid(), ());
  size_t currentIndex = std::min(m_current.m_ind + 1, m_poly.GetSize() - 1);
  m2::PointD point = m_poly.GetPoint(currentIndex);
  for (; currentIndex < m_poly.GetSize() - 1; point = m_poly.GetPoint(++currentIndex))
    if (mercator::DistanceOnEarth(point, m_current.m_pt) > toleranceM)
      break;

  pt = point;
}

Iter FollowedPolyline::GetClosestMatchingProjectionInInterval(m2::RectD const & posRect, size_t startIdx,
                                                              size_t endIdx) const
{
  CHECK_LESS_OR_EQUAL(endIdx, m_segProj.size(), ());
  CHECK_LESS_OR_EQUAL(startIdx, endIdx, ());

  Iter nearestIter;
  double minDist = std::numeric_limits<double>::max();

  m2::PointD const currPos = posRect.Center();

  for (size_t i = startIdx; i < endIdx; ++i)
  {
    m2::PointD const pt = m_segProj[i].ClosestPointTo(currPos);

    if (!posRect.IsPointInside(pt))
      continue;

    double const dp = mercator::DistanceOnEarth(pt, currPos);
    if (dp >= minDist)
      continue;

    nearestIter = Iter(pt, i);
    minDist = dp;
  }

  return nearestIter;
}
}  //  namespace routing
