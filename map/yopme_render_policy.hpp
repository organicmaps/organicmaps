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

private:
  shared_ptr<Drawer> m_offscreenDrawer;
};
