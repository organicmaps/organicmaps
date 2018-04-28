#pragma once

#include "geometry/point2d.hpp"

#include "base/base.hpp"
#include "base/stl_add.hpp"
#include "base/logging.hpp"

#include "std/iterator.hpp"
#include "std/algorithm.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

// Polyline simplification algorithms.
//
// SimplifyXXX() should be used to simplify polyline for a given epsilon.
// (!) They do not include the last point to the simplification, the calling side should do it.

namespace impl
{
///@name This functions take input range NOT like STL does: [first, last].
//@{
template <typename DistanceF, typename IterT>
pair<double, IterT> MaxDistance(IterT first, IterT last, DistanceF & dist)
{
  pair<double, IterT> res(0.0, last);
  if (distance(first, last) <= 1)
    return res;

  dist.SetBounds(m2::PointD(*first), m2::PointD(*last));
  for (IterT i = first + 1; i != last; ++i)
  {
    double const d = dist(m2::PointD(*i));
    if (d > res.first)
    {
      res.first = d;
      res.second = i;
    }
  }

  return res;
}

// Actual SimplifyDP implementation.
template <typename DistanceF, typename IterT, typename OutT>
void SimplifyDP(IterT first, IterT last, double epsilon, DistanceF & dist, OutT & out)
{
  pair<double, IterT> maxDist = impl::MaxDistance(first, last, dist);
  if (maxDist.second == last || maxDist.first < epsilon)
  {
    out(*last);
  }
  else
  {
    impl::SimplifyDP(first, maxDist.second, epsilon, dist, out);
    impl::SimplifyDP(maxDist.second, last, epsilon, dist, out);
  }
}
//@}

struct SimplifyOptimalRes
{
  SimplifyOptimalRes() : m_PointCount(-1U) {}
  SimplifyOptimalRes(int32_t nextPoint, uint32_t pointCount)
    : m_NextPoint(nextPoint), m_PointCount(pointCount) {}

  int32_t m_NextPoint;
  uint32_t m_PointCount;
};
}  // namespace impl

// Douglas-Peucker algorithm for STL-like range [beg, end).
// Iteratively includes the point with max distance form the current simplification.
// Average O(n log n), worst case O(n^2).
template <typename DistanceF, typename IterT, typename OutT>
void SimplifyDP(IterT beg, IterT end, double epsilon, DistanceF dist, OutT out)
{
  if (beg != end)
  {
    out(*beg);
    impl::SimplifyDP(beg, end-1, epsilon, dist, out);
  }
}

// Dynamic programming near-optimal simplification.
// Uses O(n) additional memory.
// Worst case O(n^3) performance, average O(n*k^2), where k is kMaxFalseLookAhead - parameter,
// which limits the number of points to try, that produce error > epsilon.
// Essentially, it's a trade-off between optimality and performance.
// Values around 20 - 200 are reasonable.
template <typename DistanceF, typename IterT, typename OutT>
void SimplifyNearOptimal(int kMaxFalseLookAhead, IterT beg, IterT end,
                         double epsilon, DistanceF dist, OutT out)
{
  int32_t const n = static_cast<int32_t>(end - beg);
  if (n <= 2)
  {
    for (IterT it = beg; it != end; ++it)
      out(*it);
    return;
  }

  vector<impl::SimplifyOptimalRes> F(n);
  F[n - 1] = impl::SimplifyOptimalRes(n, 1);
  for (int32_t i = n - 2; i >= 0; --i)
  {
    for (int32_t falseCount = 0, j = i + 1; j < n && falseCount < kMaxFalseLookAhead; ++j)
    {
      uint32_t const newPointCount = F[j].m_PointCount + 1;
      if (newPointCount < F[i].m_PointCount)
      {
          if (impl::MaxDistance(beg + i, beg + j, dist).first < epsilon)
          {
            F[i].m_NextPoint = j;
            F[i].m_PointCount = newPointCount;
          }
          else
          {
            ++falseCount;
          }
      }
    }
  }

  for (int32_t i = 0; i < n; i = F[i].m_NextPoint)
    out(*(beg + i));
}

// Additional points filter to use in simplification.
// SimplifyDP can produce points that define degenerate triangle.
template <class DistanceF, class PointT>
class AccumulateSkipSmallTrg
{
  DistanceF & m_dist;
  vector<PointT> & m_vec;
  double m_eps;

public:
  AccumulateSkipSmallTrg(DistanceF & dist, vector<PointT> & vec, double eps)
    : m_dist(dist), m_vec(vec), m_eps(eps)
  {
  }

  void operator() (PointT const & p) const
  {
    // remove points while they make linear triangle with p
    size_t count;
    while ((count = m_vec.size()) >= 2)
    {
      m_dist.SetBounds(m2::PointD(m_vec[count-2]), m2::PointD(p));
      if (m_dist(m2::PointD(m_vec[count-1])) < m_eps)
        m_vec.pop_back();
      else
        break;
    }

    m_vec.push_back(p);
  }
};
