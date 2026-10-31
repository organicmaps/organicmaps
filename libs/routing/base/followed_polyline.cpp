#include "followed_polyline.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace routing
{
using Iter = routing::FollowedPolyline::Iter;

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
  for (size_t i = 0; i < n; ++i)
  {
    m2::PointD const & p1 = m_poly.GetPoint(i);
    m2::PointD const & p2 = m_poly.GetPoint(i + 1);

    dist += mercator::DistanceOnEarth(p1, p2);

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

bool FollowedPolyline::UpdateTrackMatchingProjection(m2::RectD const & posRect, double minProgressM,
                                                     double maxProgressM, std::optional<m2::PointD> const & direction,
                                                     double progressToleranceM, bool advanceOnForwardChain)
{
  ASSERT(m_current.IsValid(), ());
  auto const currentDistance = GetDistanceFromStartMeters();
  size_t const start =
      std::lower_bound(m_segDistance.begin(), m_segDistance.end(), minProgressM) - m_segDistance.begin();
  size_t nextSegment = m_current.m_ind + 1;
  while (nextSegment < m_segProj.size() &&
         m_poly.GetPoint(nextSegment).EqualDxDy(m_poly.GetPoint(nextSegment + 1), mercator::kPointEqualityEps))
    ++nextSegment;
  bool const confirmedTurn =
      mercator::DistanceOnEarth(m_current.m_pt, m_poly.GetPoint(m_current.m_ind + 1)) <= progressToleranceM;
  size_t forwardEnd = m_current.m_ind;
  if (advanceOnForwardChain)
  {
    size_t referenceIndex = m_current.m_ind;
    while (referenceIndex + 1 < m_segProj.size() &&
           m_poly.GetPoint(referenceIndex).EqualDxDy(m_poly.GetPoint(referenceIndex + 1), mercator::kPointEqualityEps))
      ++referenceIndex;
    auto const reference = m_poly.GetPoint(referenceIndex + 1) - m_poly.GetPoint(referenceIndex);
    for (size_t i = m_current.m_ind; i < m_segProj.size(); ++i)
    {
      if (i > 0 && m_segDistance[i - 1] > maxProgressM)
        break;
      auto const delta = m_poly.GetPoint(i + 1) - m_poly.GetPoint(i);
      // A near-straight chain cannot complete a loop or skip a turnaround.
      if (m2::DotProduct(reference, delta) < 0.9 * reference.Length() * delta.Length())
        break;
      forwardEnd = i;
    }
  }
  Iter best;
  double bestSignedChange = 0.0;
  double bestDistance = std::numeric_limits<double>::max();
  double bestAlignment = -2.0;
  double bestProgressChange = std::numeric_limits<double>::max();
  for (size_t i = start; i < m_segProj.size(); ++i)
  {
    double const segmentStart = i == 0 ? 0.0 : m_segDistance[i - 1];
    if (segmentStart > maxProgressM)
      break;
    auto const & from = m_poly.GetPoint(i);
    auto const delta = m_poly.GetPoint(i + 1) - from;
    double fraction =
        delta.SquaredLength() > 0.0 ? m2::DotProduct(posRect.Center() - from, delta) / delta.SquaredLength() : 0.0;
    fraction = std::clamp(fraction, 0.0, 1.0);
    auto const point = from + delta * fraction;
    double const progress = segmentStart + mercator::DistanceOnEarth(from, point);
    if (progress < minProgressM || progress > maxProgressM)
      continue;
    if (!posRect.IsPointInside(point))
      continue;

    double const distance = mercator::DistanceOnEarth(point, posRect.Center());
    double const signedChange = progress - currentDistance;
    double const progressChange = std::fabs(signedChange);
    double const alignment = direction && delta.SquaredLength() > 0.0
                               ? m2::DotProduct(delta, *direction) / delta.Length() * (signedChange < 0.0 ? -1.0 : 1.0)
                               : 0.0;
    // GPS uncertainty makes nearby candidates spatially equivalent. Prefer the nearest
    // source occurrence, then use movement in its actual direction to break close ties.
    double const spatialTolerance = std::max(0.5, progressToleranceM);
    bool const equivalentDistance = std::fabs(distance - bestDistance) <= spatialTolerance;
    bool const closerProgress = progressChange < bestProgressChange - progressToleranceM;
    bool const comparableProgress = std::fabs(progressChange - bestProgressChange) <= progressToleranceM;
    bool const betterDirection = alignment > bestAlignment + 0.1;
    bool const sameDirection = std::fabs(alignment - bestAlignment) <= 0.1;
    // An observed corner permits forward continuity onto its adjacent edge. Do not
    // infer unobserved corners to prefer a later lap over genuine backtracking.
    bool const forwardContinuity = signedChange > 0.0 && bestSignedChange < 0.0 && confirmedTurn && i <= nextSegment;
    // Dense recordings must advance along the same occurrence rather than freeze at
    // each short segment. Only a changed raw fix enables this local continuation.
    bool const closerForward = advanceOnForwardChain && best.IsValid() && signedChange > 0.0 &&
                               bestSignedChange >= 0.0 && i >= m_current.m_ind && i <= forwardEnd &&
                               best.m_ind >= m_current.m_ind && best.m_ind <= forwardEnd && distance < bestDistance;
    bool const preferOccurrence =
        closerProgress ||
        (comparableProgress &&
         (betterDirection || (sameDirection && (forwardContinuity || progressChange < bestProgressChange))));
    if (!best.IsValid() || closerForward || distance < bestDistance - spatialTolerance ||
        (equivalentDistance && preferOccurrence))
    {
      best = Iter(point, i);
      bestDistance = distance;
      bestAlignment = alignment;
      bestProgressChange = progressChange;
      bestSignedChange = signedChange;
    }
  }
  if (!best.IsValid())
    return false;
  m_current = best;
  return true;
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
