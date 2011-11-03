#pragma once

#include "render_policy.hpp"
#include "render_queue.hpp"
#include "../geometry/point2d.hpp"

class WindowHandle;

class RenderPolicyMT : public RenderPolicy
{
protected:

  RenderQueue m_renderQueue;
  bool m_DoAddCommand;
  bool m_DoSynchronize;

public:
  RenderPolicyMT(shared_ptr<WindowHandle> const & wh,
                 RenderPolicy::TRenderFn const & renderFn);

  void Initialize(shared_ptr<yg::gl::RenderContext> const & rc,
                  shared_ptr<yg::ResourceManager> const & rm);

  void BeginFrame(shared_ptr<PaintEvent> const & e,
                  ScreenBase const & s);

  void DrawFrame(shared_ptr<PaintEvent> const & e,
                 ScreenBase const & s);

  void EndFrame(shared_ptr<PaintEvent> const & e,
                ScreenBase const & s);

  m2::RectI const OnSize(int w, int h);

  void StartDrag();
  void StopDrag();

  void StartScale();
  void StopScale();

  RenderQueue & GetRenderQueue();
  void SetNeedSynchronize(bool flag);

};
