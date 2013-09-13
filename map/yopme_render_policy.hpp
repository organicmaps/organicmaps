#pragma once

#include "render_policy.hpp"
#include "../std/shared_ptr.hpp"

class Drawer;
class YopmeRP : public RenderPolicy
{
public:
  YopmeRP(RenderPolicy::Params const & p);
  virtual void DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);

  virtual void OnSize(int w, int h);

  void DrawApiPin(bool isNeed, m2::PointD const & point);
  void DrawMyLocation(bool isNeed, m2::PointD const & point);

private:
  shared_ptr<Drawer> m_offscreenDrawer;
  bool m_drawApiPin;
  m2::PointD m_apiPinPoint; // in pixels
  bool m_drawMyPosition;
  m2::PointD m_myPositionPoint; // in pixels
};
