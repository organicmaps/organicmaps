#pragma once

#include "coding/geometry_coding.hpp"

#include "geometry/parametrized_segment.hpp"
#include "geometry/point2d.hpp"
#include "geometry/simplification.hpp"

#include "indexer/scales.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

class FeatureBuilder1;

namespace feature
{
class CalculateMidPoints
{
public:
  using CellAndOffset = std::pair<uint64_t, uint64_t>;

  CalculateMidPoints() = default;

  void operator()(FeatureBuilder1 const & ft, uint64_t pos);
  bool operator()(m2::PointD const & p);

  m2::PointD GetCenter() const;
  std::vector<CellAndOffset> const & GetVector() const { return m_vec; }

  void Sort();

private:
  m2::PointD m_midLoc;
  m2::PointD m_midAll;
  size_t m_locCount = 0;
  size_t m_allCount = 0;
  uint32_t m_coordBits = serial::GeometryCodingParams().GetCoordBits();
  std::vector<CellAndOffset> m_vec;
};

template <class Point>
inline bool ArePointsEqual(Point const & p1, Point const & p2)
{
  return p1 == p2;
}

template <>
inline bool ArePointsEqual<m2::PointD>(m2::PointD const & p1, m2::PointD const & p2)
{
  return AlmostEqualULPs(p1, p2);
}

class SegmentWithRectBounds
{
public:
  SegmentWithRectBounds(m2::RectD const & rect, double eps, m2::PointD const & p0,
                        m2::PointD const & p1)
    : m_rect(rect), m_eps(eps), m_segment(p0, p1)
  {
  }

  double SquaredDistanceToPoint(m2::PointD const & p) const
  {
    if (my::AlmostEqualAbs(p.x, m_rect.minX(), m_eps) ||
        my::AlmostEqualAbs(p.x, m_rect.maxX(), m_eps) ||
        my::AlmostEqualAbs(p.y, m_rect.minY(), m_eps) ||
        my::AlmostEqualAbs(p.y, m_rect.maxY(), m_eps))
    {
      // Points near rect should be in a result simplified vector.
      return std::numeric_limits<double>::max();
    }

    return m_segment.SquaredDistanceToPoint(p);
  }

private:
  m2::RectD const & m_rect;
  double m_eps;
  m2::ParametrizedSegment<m2::PointD> m_segment;
};

class SegmentWithRectBoundsFactory
{
public:
  explicit SegmentWithRectBoundsFactory(m2::RectD const & rect) : m_rect(rect) {}

  SegmentWithRectBounds operator()(m2::PointD const & p0, m2::PointD const & p1)
  {
    return SegmentWithRectBounds(m_rect, m_eps, p0, p1);
  }

  double GetEpsilon() const { return m_eps; }

private:
  m2::RectD const & m_rect;
  // 5.0E-7 is near with minimal epsilon when integer points are different
  // PointDToPointU(x, y) != PointDToPointU(x + m_eps, y + m_eps)
  double m_eps = 5.0E-7;
};

template <class DistanceFact, class PointsContainer>
void SimplifyPoints(DistanceFact distFact, int level, PointsContainer const & in,
                    PointsContainer & out)
{
  if (in.size() < 2)
    return;

  double const eps = std::pow(scales::GetEpsilonForSimplify(level), 2);

  SimplifyNearOptimal(20, in.begin(), in.end(), eps, distFact,
                      AccumulateSkipSmallTrg<DistanceFact, m2::PointD>(distFact, out, eps));

  CHECK_GREATER(out.size(), 1, ());
  CHECK(ArePointsEqual(in.front(), out.front()), ());
  CHECK(ArePointsEqual(in.back(), out.back()), ());
}
}  // namespace feature
