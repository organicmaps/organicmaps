#pragma once

#include "animation/model_view_animation.hpp"

#include "geometry/any_rect2d.hpp"

namespace df
{

class KineticScroller
{
public:
  KineticScroller();

  void InitGrab(ScreenBase const & modelView, double timeStamp);
  void GrabViewRect(ScreenBase const & modelView, double timeStamp);
  void CancelGrab();
  unique_ptr<BaseModelViewAnimation> CreateKineticAnimation(ScreenBase const & modelView);

private:
  double m_lastTimestamp;
  m2::AnyRectD m_lastRect;
  m2::PointD m_direction;
};

} // namespace df
