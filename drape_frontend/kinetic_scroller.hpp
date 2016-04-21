#pragma once

#include "animation_system.h"

#include "drape/pointers.hpp"

#include "geometry/any_rect2d.hpp"

namespace df
{

class KineticScroller
{
public:
  KineticScroller();

  void InitGrab(ScreenBase const & modelView, double timeStamp);
  bool IsActive() const;
  void GrabViewRect(ScreenBase const & modelView, double timeStamp);
  void CancelGrab();
  drape_ptr<Animation> CreateKineticAnimation(ScreenBase const & modelView);

private:
  double m_lastTimestamp;
  m2::AnyRectD m_lastRect;
  m2::PointD m_direction;
};

} // namespace df
