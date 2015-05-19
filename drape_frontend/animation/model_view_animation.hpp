#pragma once

#include "drape_frontend/animation/base_interpolator.hpp"
#include "drape_frontend/animation/interpolations.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/screenbase.hpp"

namespace df
{

class ModelViewAnimation : public BaseInterpolator
{
public:
  ModelViewAnimation(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect, double duration);
  m2::AnyRectD GetCurrentRect() const;
  m2::AnyRectD GetTargetRect() const;

  static double GetDuration(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect, ScreenBase const & convertor);

private:
  InterpolateAnyRect m_interpolator;
};

} // namespace df
