#include "drape_frontend/animation/interpolations.hpp"

#include "drape_frontend/animation_constants.hpp"

#include "geometry/angles.hpp"

namespace df
{

double InterpolateDouble(double startV, double endV, double t)
{
  if (t == 1.0)
    return endV;
  return startV + (endV - startV) * t;
}

m2::PointD InterpolatePoint(m2::PointD const & startPt, m2::PointD const & endPt, double t)
{
  if (t == 1.0)
    return endPt;
  return startPt + (endPt - startPt) * t;
}

double InterpolateAngle(double startAngle, double endAngle, double t)
{
  if (t == 1.0 || startAngle == endAngle)
    return endAngle;
  if (t == 0.0)
    return startAngle;
  return startAngle + ang::GetShortestDistance(startAngle, endAngle) * t;
}

double GetPopUpScale(double t)
{
  if (t < kPopUpOvershootTime)
    return InterpolateDouble(0.0, kPopUpOvershoot, t / kPopUpOvershootTime);
  if (t < kPopUpUndershootTime)
    return InterpolateDouble(kPopUpOvershoot, kPopUpUndershoot,
                             (t - kPopUpOvershootTime) / (kPopUpUndershootTime - kPopUpOvershootTime));
  return InterpolateDouble(kPopUpUndershoot, 1.0, (t - kPopUpUndershootTime) / (1.0 - kPopUpUndershootTime));
}

}  // namespace df
