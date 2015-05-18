#include "modelview_complex_animation.hpp"

namespace df
{

ModelViewComplexAnimation::ModelViewComplexAnimation(m2::AnyRectD const & startRect, m2::AnyRectD const & endRect, double duration)
  : BaseModeViewAnimation(duration)
  , m_interpolator(startRect, endRect)
{
}

void ModelViewComplexAnimation::Apply(df::Navigator & navigator)
{
  navigator.SetFromRect(m_interpolator.Interpolate(GetT()));
}

}
