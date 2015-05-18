#pragma once

#include "drape_frontend/animation/base_modelview_animation.hpp"
#include "drape_frontend/animation/interpolations.hpp"

namespace df
{

class ModelViewComplexAnimation : public BaseModeViewAnimation
{
public:
  ModelViewComplexAnimation(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect, double duration);
  void Apply(Navigator & navigator) override;

private:
  InterpolateAnyRect m_interpolator;
};

}
