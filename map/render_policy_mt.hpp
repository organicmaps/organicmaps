#pragma once

#include "render_policy.hpp"
#include "render_queue.hpp"
#include "drawer_yg.hpp"

#include "../geometry/point2d.hpp"
#include "../std/scoped_ptr.hpp"

class WindowHandle;
class VideoTimer;

class RenderPolicyMT : public RenderPolicy
{
protected:

  scoped_ptr<RenderQueue> m_renderQueue;
  bool m_DoAddCommand;

public:

  RenderPolicyMT(VideoTimer * videoTimer,
                 bool useDefaultFB,
                 yg::ResourceManager::Params const & rmParams,
                 shared_ptr<yg::gl::RenderContext> const & primaryRC);

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

  bool IsEmptyModel() const;

  RenderQueue & GetRenderQueue();

  void SetRenderFn(TRenderFn renderFn);
};
