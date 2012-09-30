#pragma once

#include "../geometry/any_rect2d.hpp"
#include "angle_interpolation.hpp"
#include "segment_interpolation.hpp"
#include "value_interpolation.hpp"

namespace anim
{
  class AnyRectInterpolation : public Task
  {
  private:

    double m_interval;

    AngleInterpolation m_angleInt;
    double m_curAngle;

    SegmentInterpolation m_segmentInt;
    m2::PointD m_curCenter;

    ValueInterpolation m_sizeXInt;
    double m_curSizeX;

    ValueInterpolation m_sizeYInt;
    double m_curSizeY;

    m2::AnyRectD m_startRect;
    m2::AnyRectD m_endRect;
    m2::AnyRectD & m_outRect;

    double m_startTime;

  public:

    AnyRectInterpolation(m2::AnyRectD const & startRect,
                         m2::AnyRectD const & endRect,
                         double rotationSpeed,
                         m2::AnyRectD & outRect);

    void OnStart(double ts);
    void OnStep(double ts);
    void OnEnd(double ts);
  };
}
