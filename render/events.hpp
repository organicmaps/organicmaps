#pragma once

#include "geometry/screenbase.hpp"
#include "geometry/point2d.hpp"

#include "base/commands_queue.hpp"


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
    m_Angle = atan2(y2 - y1, x2 - x1);
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

#ifndef USE_DRAPE
class Drawer;

class PaintEvent
{
  Drawer * m_drawer;
  core::CommandsQueue::Environment const * m_env;
  bool m_isCancelled;
  bool m_isEmptyDrawing;

public:
  PaintEvent(Drawer * drawer, core::CommandsQueue::Environment const * env = 0);

  Drawer * drawer() const;
  void cancel();
  bool isCancelled() const;
  bool isEmptyDrawing() const;
  void setIsEmptyDrawing(bool flag);
};

class PaintOverlayEvent
{
public:
  PaintOverlayEvent(Drawer * drawer, ScreenBase const & modelView)
    : m_drawer(drawer), m_modelView(modelView) {}

  ScreenBase const & GetModelView() const { return m_modelView; }
  Drawer * GetDrawer() const { return m_drawer; }
  m2::RectD const & GetClipRect() const { return m_modelView.ClipRect(); }

private:
  Drawer * m_drawer;
  ScreenBase m_modelView;
};
#endif // USE_DRAPE
