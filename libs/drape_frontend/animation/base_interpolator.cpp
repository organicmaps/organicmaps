#include "drape_frontend/animation/base_interpolator.hpp"
#include "drape_frontend/animation/interpolation_holder.hpp"

namespace df
{
BaseInterpolator::BaseInterpolator(double duration, double delay)
  : Animation(AnimationClock::now() +
                  std::chrono::duration_cast<AnimationClock::duration>(std::chrono::duration<double>(delay)),
              duration)
{
  InterpolationHolder::Instance().RegisterInterpolator(this);
}

BaseInterpolator::~BaseInterpolator()
{
  InterpolationHolder::Instance().DeregisterInterpolator(this);
}
}  // namespace df
