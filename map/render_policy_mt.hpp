#pragma once

#include "render_policy.hpp"
#include "render_queue.hpp"
#include "../geometry/point2d.hpp"

class WindowHandle;

class RenderPolicyMT : public RenderPolicy
{
private:

  RenderQueue m_renderQueue;
  bool m_DoAddCommand;

public:
  RenderPolicyMT(shared_ptr<WindowHandle> const & wh,
                 RenderPolicy::TRenderFn const & renderFn);

  void Initialize(shared_ptr<yg::gl::RenderContext> const & rc,
                  shared_ptr<yg::ResourceManager> const & rm);

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent,
                 ScreenBase const & screenBase);

  m2::RectI const OnSize(int w, int h);

  void StartDrag(m2::PointD const & pt, double timeInSec);
  void StopDrag(m2::PointD const & pt, double timeInSec);

  void StartScale(m2::PointD const & pt1, m2::PointD const & pt2, double timeInSec);
  void StopScale(m2::PointD const & pt1, m2::PointD const & pt2, double timeInSec);
};
