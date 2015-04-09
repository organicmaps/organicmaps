#pragma once

#include "anim/segment_interpolation.hpp"

class Framework;

class MoveScreenTask : public anim::SegmentInterpolation
{
private:

  Framework * m_framework;
  m2::PointD m_outPt;

public:

  MoveScreenTask(Framework * framework,
                 m2::PointD const & startPt,
                 m2::PointD const & endPt,
                 double interval);

  void OnStep(double ts);
  void OnEnd(double ts);

  bool IsVisual() const;
};
