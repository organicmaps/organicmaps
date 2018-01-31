#pragma once
#include "generator/generate_info.hpp"

#include "geometry/distance.hpp"
#include "geometry/point2d.hpp"
#include "geometry/simplification.hpp"

#include "indexer/scales.hpp"

#include <string>

namespace feature
{
/// Final generation of data from input feature-dat-file.
/// @param path - path to folder with countries;
/// @param name - name of generated country;
bool GenerateFinalFeatures(feature::GenerateInfo const & info, std::string const & name,
                           int mapType);

template <class PointT>
inline bool are_points_equal(PointT const & p1, PointT const & p2)
{
  return p1 == p2;
}

template <>
inline bool are_points_equal<m2::PointD>(m2::PointD const & p1, m2::PointD const & p2)
{
  return AlmostEqualULPs(p1, p2);
}

class BoundsDistance : public m2::DistanceToLineSquare<m2::PointD>
{
  m2::RectD const & m_rect;
  double m_eps;

public:
  BoundsDistance(m2::RectD const & rect) : m_rect(rect), m_eps(5.0E-7)
  {
    // 5.0E-7 is near with minimal epsilon when integer points are different
    // PointD2PointU(x, y) != PointD2PointU(x + m_eps, y + m_eps)
  }

  double GetEpsilon() const { return m_eps; }

  double operator()(m2::PointD const & p) const
  {
    if (fabs(p.x - m_rect.minX()) <= m_eps || fabs(p.x - m_rect.maxX()) <= m_eps ||
        fabs(p.y - m_rect.minY()) <= m_eps || fabs(p.y - m_rect.maxY()) <= m_eps)
    {
      // points near rect should be in a result simplified vector
      return std::numeric_limits<double>::max();
    }

    return m2::DistanceToLineSquare<m2::PointD>::operator()(p);
  }
};

template <class DistanceT, class PointsContainerT>
void SimplifyPoints(DistanceT dist, PointsContainerT const & in, PointsContainerT & out, int level)
{
  if (in.size() >= 2)
  {
    double const eps = my::sq(scales::GetEpsilonForSimplify(level));

    SimplifyNearOptimal(20, in.begin(), in.end(), eps, dist,
                        AccumulateSkipSmallTrg<DistanceT, m2::PointD>(dist, out, eps));

    CHECK_GREATER(out.size(), 1, ());
    CHECK(are_points_equal(in.front(), out.front()), ());
    CHECK(are_points_equal(in.back(), out.back()), ());
  }
}
}  // namespace feature
