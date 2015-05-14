#pragma once

#include "drape_frontend/animation/base_interpolator.hpp"
#include "drape_frontend/navigator.hpp"

namespace df
{

class BaseModeViewAnimation : public BaseInterpolator
{
public:
  BaseModeViewAnimation(double duration) : BaseInterpolator(duration) {}
  virtual void Apply(Navigator & navigator) = 0;
};

}
