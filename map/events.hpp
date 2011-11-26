#pragma once

#include "../base/commands_queue.hpp"

#include "../geometry/point2d.hpp"

class DragEvent
{
  m2::PointD m_pt;
public:
  DragEvent(double x, double y) : m_pt(x, y) {}
  inline m2::PointD const & Pos() const { return m_pt; }
};

class RotateEvent
{
  double m_Angle;
public:
  RotateEvent(double x1, double y1, double x2, double y2)
  {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double len = sqrt(dx * dx + dy * dy);
    dy /= len;
    dx /= len;
    m_Angle = atan2(dy, dx);
  }

  inline double Angle() const { return m_Angle; }
};

class ScaleEvent
{
  m2::PointD m_Pt1, m_Pt2;
public:
  ScaleEvent(double x1, double y1, double x2, double y2) : m_Pt1(x1, y1), m_Pt2(x2, y2) {}
  inline m2::PointD const & Pt1() const { return m_Pt1; }
  inline m2::PointD const & Pt2() const { return m_Pt2; }
};

class ScaleToPointEvent
{
  m2::PointD m_Pt1;
  double m_factor;
public:
  ScaleToPointEvent(double x1, double y1, double factor) : m_Pt1(x1, y1), m_factor(factor) {}
  inline m2::PointD const & Pt() const { return m_Pt1; }
  inline double ScaleFactor() const { return m_factor; }
};

class DrawerYG;

class PaintEvent
{

  DrawerYG * m_drawer;
  core::CommandsQueue::Environment const * m_env;
  bool m_isCancelled;

public:

  PaintEvent(DrawerYG * drawer, core::CommandsQueue::Environment const * env = 0)
    : m_drawer(drawer), m_env(env), m_isCancelled(false)
  {}

  DrawerYG * drawer() const
  {
    return m_drawer;
  }

  void Cancel()
  {
    ASSERT(m_env == 0, ());
    m_isCancelled = true;
  }

  bool isCancelled() const
  {
    if (m_env)
      return m_env->isCancelled();
    else
      return m_isCancelled;
  }
};
