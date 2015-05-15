#pragma once

#include "map/render_policy.hpp"
#include "std/shared_ptr.hpp"

class Drawer;
class YopmeRP : public RenderPolicy
{
public:
  YopmeRP(RenderPolicy::Params const & p);
  virtual void DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);

  virtual void OnSize(int w, int h);

  void SetDrawingApiPin(bool isNeed, m2::PointD const & point);
  void SetDrawingMyLocation(bool isNeed, m2::PointD const & point);

private:
  static void DrawCircle(graphics::Screen * pScreen, m2::PointD const & pt);
  static void InsertOverlayCross(m2::PointD pivot, shared_ptr<graphics::OverlayStorage> const & overlayStorage);

  shared_ptr<GPUDrawer> m_offscreenDrawer;
  bool m_drawApiPin;
  m2::PointD m_apiPinPoint; // in pixels
  bool m_drawMyPosition;
  m2::PointD m_myPositionPoint; // in pixels
};
