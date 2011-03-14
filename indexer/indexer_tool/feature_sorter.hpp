#pragma once

#include "../../geometry/point2d.hpp"
#include "../../geometry/simplification.hpp"
#include "../../geometry/distance.hpp"

#include "../../indexer/scales.hpp"

#include "../../std/string.hpp"
#include "../../std/vector.hpp"

namespace feature
{
  /// Final generation of data from input feature-dat-file.
  /// @param[in] bSort sorts features in the given file by their mid points
  bool GenerateFinalFeatures(string const & datFile, bool bSort, bool bWorld);

  template <class PointT>
  inline bool are_points_equal(PointT const & p1, PointT const & p2)
  {
    return p1 == p2;
  }

  template <>
  inline bool are_points_equal<m2::PointD>(m2::PointD const & p1, m2::PointD const & p2)
  {
    return AlmostEqual(p1, p2);
  }

  template <class PointsContainerT>
  void SimplifyPoints(PointsContainerT const & in, PointsContainerT & out, int level)
  {
    if (in.size() >= 2)
    {
      typedef mn::DistanceToLineSquare<m2::PointD> DistanceF;
      double const eps = my::sq(scales::GetEpsilonForSimplify(level));

      SimplifyNearOptimal<DistanceF>(20, in.begin(), in.end(), eps,
                                     AccumulateSkipSmallTrg<DistanceF, m2::PointD>(out, eps));

      ASSERT_GREATER ( out.size(), 1, () );
      ASSERT ( are_points_equal(in.front(), out.front()), () );
      ASSERT ( are_points_equal(in.back(), out.back()), () );

#ifdef DEBUG
      //for (size_t i = 2; i < out.size(); ++i)
      //{
      //  double const dist = DistanceF(out[i-2], out[i])(out[i-1]);
      //  ASSERT ( dist >= eps, (dist, eps, in) );
      //}
#endif
    }
  }
}
