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
  bool GenerateFinalFeatures(string const & datFile, bool bSort);

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
      SimplifyNearOptimal<mn::DistanceToLineSquare<typename PointsContainerT::value_type> >(
          20, in.begin(), in.end()-1, my::sq(scales::GetEpsilonForSimplify(level)),
          MakeBackInsertFunctor(out));

      switch (out.size())
      {
      case 0:
        out.push_back(in.front());
        // no break
      case 1:
        out.push_back(in.back());
        break;
      default:
        if (!are_points_equal(out.back(), in.back()))
          out.push_back(in.back());
      }
    }
  }
}
