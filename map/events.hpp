#pragma once
#include "../geometry/point2d.hpp"

#include "../std/shared_ptr.hpp"

#include "../base/commands_queue.hpp"

#include "../base/start_mem_debug.hpp"


class Event
{
public:
  Event() {}
};

class DragEvent
{
  m2::PointD m_pt;
public:
  DragEvent(double x, double y) : m_pt(x, y) {}
  m2::PointD Pos() const { return m_pt; }
};

class ScaleEvent
{
  m2::PointD m_Pt1, m_Pt2;
public:
  ScaleEvent(double x1, double y1, double x2, double y2) : m_Pt1(x1, y1), m_Pt2(x2, y2) {}
  m2::PointD Pt1() const { return m_Pt1; }
  m2::PointD Pt2() const { return m_Pt2; }
};

class ScaleToPointEvent
{
  m2::PointD m_Pt1;
  double m_factor;
public:
  ScaleToPointEvent(double x1, double y1, double factor) : m_Pt1(x1, y1), m_factor(factor) {}
  m2::PointD Pt() const { return m_Pt1; }
  double ScaleFactor() const { return m_factor; }
};

class DrawerYG;

class PaintEvent : public Event
{

  DrawerYG * m_drawer;
  core::CommandsQueue::Environment const * m_env;

public:

  PaintEvent(DrawerYG * drawer, core::CommandsQueue::Environment const * env = 0)
    : m_drawer(drawer), m_env(env)
  {}

  DrawerYG * drawer() const
  {
    return m_drawer;
  }

  bool isCancelled() const
  {
    return (m_env && m_env->IsCancelled());
  }
};

#include "../base/stop_mem_debug.hpp"
