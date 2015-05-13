#include "drape_frontend/animation/interpolations.hpp"

namespace df
{

m2::PointD Interpolate(m2::PointD const & startPt, m2::PointD const & endPt, double t)
{
  m2::PointD diff = endPt - startPt;
  return startPt + diff * t;
}

} // namespace df
