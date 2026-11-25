#pragma once

#include "drape_frontend/animation/animation.hpp"

#include "geometry/point2d.hpp"

namespace df
{
struct ArrowPose
{
  m2::PointD m_position;
  double m_azimuth;
};

class ArrowAnimation : public Animation
{
public:
  ArrowAnimation(ArrowPose const & start, ArrowPose const & end, AnimationClock::time_point requestedAt,
                 double moveDuration, double rotateDuration);
  ArrowPose GetPose() const;

private:
  ArrowPose const m_start;
  ArrowPose const m_end;
  double const m_moveDuration;
  double const m_rotateDuration;
};

}  // namespace df
