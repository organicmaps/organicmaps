#pragma once

#include "queued_render_policy.hpp"
#include "../yg/screen.hpp"
#include "../base/threaded_list.hpp"
#include "../std/scoped_ptr.hpp"

class RenderQueue;
class WindowHandle;

class PartialRenderPolicy : public QueuedRenderPolicy
{
private:

  typedef QueuedRenderPolicy base_t;

  scoped_ptr<RenderQueue> m_renderQueue;
  bool m_DoAddCommand;
  bool m_DoSynchronize;

public:

  PartialRenderPolicy(VideoTimer * videoTimer,
                      bool useDefaultFB,
                      yg::ResourceManager::Params const & rmParams,
                      shared_ptr<yg::gl::RenderContext> const & primaryRC);

  ~PartialRenderPolicy();

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent,
                 ScreenBase const & screenBase);

  void EndFrame(shared_ptr<PaintEvent> const & paintEvent,
                ScreenBase const & screenBase);

  m2::RectI const OnSize(int w, int h);

  bool IsEmptyModel() const;

  void StartDrag();
  void StopDrag();

  void StartScale();
  void StopScale();

  RenderQueue & GetRenderQueue();

  void SetRenderFn(TRenderFn renderFn);
  void SetEmptyModelFn(TEmptyModelFn const & checkFn);
};
