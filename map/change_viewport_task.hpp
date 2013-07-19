#pragma once

#include "../anim/anyrect_interpolation.hpp"

class Framework;

class ChangeViewportTask : public anim::AnyRectInterpolation
{
  typedef anim::AnyRectInterpolation BaseT;

  Framework * m_framework;
  m2::AnyRectD m_outRect;

public:

  ChangeViewportTask(m2::AnyRectD const & startRect,
                     m2::AnyRectD const & endRect,
                     double rotationSpeed,
                     Framework * framework);

  void OnStep(double ts);
  void OnEnd(double ts);

  bool IsVisual() const;
};
