#pragma once

#include "drape_frontend/animation/base_modelview_animation.hpp"
#include "drape_frontend/animation/interpolations.hpp"

namespace df
{

class ModelViewAngleAnimation : public BaseModeViewAnimation
{
public:
  ModelViewAngleAnimation(double startAngle, double endAngle, double duration);
  void Apply(Navigator & navigator) override;

private:
  InerpolateAngle m_angle;
};

}
