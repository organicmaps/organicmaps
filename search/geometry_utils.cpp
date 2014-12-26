#include "geometry_utils.hpp"

#include "../indexer/mercator.hpp"


namespace search
{

double PointDistance(m2::PointD const & a, m2::PointD const & b)
{
  return MercatorBounds::DistanceOnEarth(a, b);
}

uint8_t ViewportDistance(m2::RectD const & viewport, m2::PointD const & p)
{
  if (viewport.IsPointInside(p))
    return 0;

  m2::RectD r = viewport;
  r.Scale(3);
  if (r.IsPointInside(p))
    return 1;

  r = viewport;
  r.Scale(5);
  if (r.IsPointInside(p))
    return 2;

  return 3;
}

}
