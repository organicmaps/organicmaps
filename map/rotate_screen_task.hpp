#pragma once

#include "anim/angle_interpolation.hpp"

class Framework;

class RotateScreenTask : public anim::AngleInterpolation
{
private:

  Framework * m_framework;
  double m_outAngle;

public:

  RotateScreenTask(Framework * framework,
                   double startAngle,
                   double endAngle,
                   double speed);

  void OnStep(double ts);
  void OnEnd(double ts);

  bool IsVisual() const;
};
