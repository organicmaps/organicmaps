#include "geometry_utils.hpp"

#include "indexer/mercator.hpp"
#include "indexer/scales.hpp"


namespace search
{

double PointDistance(m2::PointD const & a, m2::PointD const & b)
{
  return MercatorBounds::DistanceOnEarth(a, b);
}

bool IsEqualMercator(m2::RectD const & r1, m2::RectD const & r2, double epsMeters)
{
  double const eps = epsMeters * MercatorBounds::degreeInMetres;
  return m2::IsEqual(r1, r2, eps, eps);
}

// 12.5 - lower bound for rect when we need to inflate it
// 1.5 - inflate delta for viewport scale
// 7 - query scale depth to cache viewport features

m2::RectD GetInflatedViewport(m2::RectD const & viewport)
{
  double const level = scales::GetScaleLevelD(viewport);
  if (level < 12.5)
    return viewport;

  return scales::GetRectForLevel(level - 1.5, viewport.Center());
}

int GetQueryIndexScale(m2::RectD const & viewport)
{
  return scales::GetScaleLevel(viewport) + 7;
}

}
