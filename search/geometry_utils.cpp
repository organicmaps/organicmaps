#include "geometry_utils.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

namespace search
{
double PointDistance(m2::PointD const & a, m2::PointD const & b)
{
  return MercatorBounds::DistanceOnEarth(a, b);
}

bool IsEqualMercator(m2::RectD const & r1, m2::RectD const & r2, double eps)
{
  return m2::IsEqual(r1, r2, eps, eps);
}

bool GetInflatedViewport(m2::RectD & viewport)
{
  // 11.5 - lower bound for rect when we need to inflate it
  // 1.5 - inflate delta for viewport scale
  // 7 - query scale depth to cache viewport features
  double level = scales::GetScaleLevelD(viewport);
  if (level < 11.5)
    return false;
  if (level > 16.5)
    level = 16.5;

  viewport = scales::GetRectForLevel(level - 1.5, viewport.Center());
  return true;
}

int GetQueryIndexScale(m2::RectD const & viewport)
{
  return scales::GetScaleLevel(viewport) + 7;
}
}  // namespace search
