#pragma once

#include "drape_frontend/animation/base_viewport_animation.hpp"

namespace df
{

class ModelViewCenterAnimation : public BaseModeViewAnimation
{
public:
  ModelViewCenterAnimation(m2::PointD const & start, m2::PointD const & end, double duration);

  void Apply(Navigator & navigator) override;

private:
  m2::PointD m_startPt;
  m2::PointD m_endPt;
};

}
