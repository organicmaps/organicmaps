#pragma once

#include "geometry/parametrized_segment.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <vector>

// Polyline simplification algorithms.

namespace simpl
{
///@name This functions take input range NOT like STL does: [first, last].
//@{
template <typename DistanceFn, typename Iter>
std::pair<double, Iter> MaxDistance(Iter first, Iter last, DistanceFn & distFn)
{
  std::pair<double, Iter> res(0.0, last);

  for (Iter i = first + 1; i != last; ++i)
  {
    double const d = distFn(*first, *last, *i);
    if (res.first < d)
    {
      res.first = d;
      res.second = i;
    }
  }

  return res;
}

// Actual SimplifyDP implementation.
template <typename DistanceFn, typename Iter, typename Out>
void SimplifyDP(Iter first, Iter last, double epsilon, DistanceFn & distFn, Out & out)
{
  if (first != last)
  {
    auto const maxDist = MaxDistance(first, last, distFn);
    if (maxDist.first >= epsilon)
    {
      simpl::SimplifyDP(first, maxDist.second, epsilon, distFn, out);
      simpl::SimplifyDP(maxDist.second, last, epsilon, distFn, out);
      return;
    }
  }
  out(*last);
}
//@}

struct SimplifyOptimalRes
{
  SimplifyOptimalRes() = default;
  SimplifyOptimalRes(int32_t nextPoint, uint32_t pointCount) : m_NextPoint(nextPoint), m_PointCount(pointCount) {}

  int32_t m_NextPoint = -1;
  uint32_t m_PointCount = -1U;
};
}  // namespace simpl

// Douglas-Peucker algorithm for STL-like range [beg, end).
// Iteratively includes the point with max distance from the current simplification.
// Average O(n log n), worst case O(n^2).
template <typename DistanceFn, typename Iter, typename Out>
void SimplifyDP(Iter beg, Iter end, double epsilon, DistanceFn distFn, Out out)
{
  if (beg != end)
  {
    out(*beg);
    simpl::SimplifyDP(beg, end - 1, epsilon, distFn, out);
  }
}

// Dynamic programming near-optimal simplification.
// Uses O(n) additional memory.
// Worst case O(n^3) performance, average O(n*k^2), where k is maxFalseLookAhead - parameter,
// which limits the number of points to try, that produce error > epsilon.
// Essentially, it's a trade-off between optimality and performance.
// Values around 20 - 200 are reasonable.
template <typename DistanceFn, typename Iter, typename Out>
void SimplifyNearOptimal(int maxFalseLookAhead, Iter beg, Iter end, double epsilon, DistanceFn distFn, Out out)
{
  int32_t const n = static_cast<int32_t>(end - beg);
  if (n <= 2)
  {
    for (Iter it = beg; it != end; ++it)
      out(*it);
    return;
  }

  std::vector<simpl::SimplifyOptimalRes> F(n);
  F[n - 1] = simpl::SimplifyOptimalRes(n, 1);
  for (int32_t i = n - 2; i >= 0; --i)
  {
    for (int32_t falseCount = 0, j = i + 1; j < n && falseCount < maxFalseLookAhead; ++j)
    {
      uint32_t const newPointCount = F[j].m_PointCount + 1;
      if (newPointCount < F[i].m_PointCount)
      {
        if (simpl::MaxDistance(beg + i, beg + j, distFn).first < epsilon)
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
template <typename DistanceFn, typename Point>
class AccumulateSkipSmallTrg
{
public:
  AccumulateSkipSmallTrg(DistanceFn & distFn, std::vector<Point> & vec, double eps)
    : m_distFn(distFn)
    , m_vec(vec)
    , m_eps(eps)
  {}

  void operator()(Point const & p) const
  {
    // remove points while they make linear triangle with p
    size_t count;
    while ((count = m_vec.size()) >= 2)
      if (m_distFn(m_vec[count - 2], p, m_vec[count - 1]) < m_eps)
        m_vec.pop_back();
      else
        break;

    m_vec.push_back(p);
  }

private:
  DistanceFn & m_distFn;
  std::vector<Point> & m_vec;
  double m_eps;
};

template <class IterT, class PointT>
void SimplifyDefault(IterT beg, IterT end, double squareEps, std::vector<PointT> & out)
{
  m2::SquaredDistanceFromSegmentToPoint distFn;
  SimplifyNearOptimal(20 /* maxFalseLookAhead */, beg, end, squareEps, distFn,
                      AccumulateSkipSmallTrg(distFn, out, squareEps));
}
