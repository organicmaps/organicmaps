#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/spline.hpp"

#include <functional>
#include <vector>

namespace m2
{
using ClipTriangleByRectResultIt = std::function<void(m2::PointD const &, m2::PointD const &, m2::PointD const &)>;

void ClipTriangleByRect(m2::RectD const & rect, m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3,
                        ClipTriangleByRectResultIt const & resultIterator);

/// Clips |path| against |rect| and back-inserts the resulting sub-splines into |out|.
/// Caller is expected to have already classified the path against the rect
/// (e.g. via the feature's limit rect) and only invoke this for the real
/// Intersect case — Inside/Outside dispatch is intentionally absent. The
/// underlying implementation is still correct for Inside/Outside paths
/// (just redundant), so passing such paths is safe but wasteful.
void ClipPathByRect(m2::RectD const & rect, std::vector<m2::PointD> const & path, std::vector<m2::SharedSpline> & out);
std::vector<m2::SharedSpline> ClipSplineByRect(m2::RectD const & rect, m2::SharedSpline const & spline);

using GuidePointsForSmooth = std::vector<std::pair<m2::PointD, m2::PointD>>;
void ClipPathByRectBeforeSmooth(m2::RectD const & rect, std::vector<m2::PointD> const & path,
                                GuidePointsForSmooth & guidePoints,
                                std::vector<std::vector<m2::PointD>> & clippedPaths);

/// Checks whether any segment of the spline actually crosses the rect.
/// @note Callers should check bbox intersection first for performance.
bool IsRealIntersect(m2::RectD const & rect, m2::Spline const & spl);

/// Iterate over spline sections that are no longer than |maxLength| in spline's coordinates (mercator).
template <class FnT>
void ForEachSection(m2::Spline const & spl, double maxLength, FnT && fn)
{
  auto const & path = spl.GetPath();
  for (size_t i = 1; i < path.size(); ++i)
  {
    auto const & p1 = path[i - 1];
    auto const & p2 = path[i];

    double const length = p1.Length(p2);
    if (length <= maxLength)  // most likely path
      fn(p1, p2);
    else
    {
      int const steps = static_cast<int>(std::ceil(length / maxLength));

      auto prev = p1;
      for (int s = 1; s <= steps; ++s)
      {
        double const t = static_cast<double>(s) / steps;
        auto const curr = p1 * (1 - t) + p2 * t;
        fn(prev, curr);
        prev = curr;
      }
    }
  }
}

}  // namespace m2
