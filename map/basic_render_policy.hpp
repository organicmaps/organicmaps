#pragma once

#include "../std/shared_ptr.hpp"

#include "render_policy.hpp"

class QueuedRenderer;
class RenderQueue;

/// This is a base class for both multithreaded and singlethreaded rendering policies
/// that uses a double buffering scheme.
/// primary OpenGL thread only blits frontBuffer(RenderState::actualTarget),
/// while another thread renders scene on the backBuffer and swaps it
/// with frontBuffer periodically
class BasicRenderPolicy : public RenderPolicy
{
protected:

  shared_ptr<QueuedRenderer> m_QueuedRenderer;
  shared_ptr<RenderQueue> m_RenderQueue;

  bool m_DoAddCommand;

public:

  BasicRenderPolicy(shared_ptr<yg::gl::RenderContext> const & primaryRC,
                    bool doSupportRotation,
                    size_t idCacheSize,
                    shared_ptr<QueuedRenderer> const & queuedRenderer = shared_ptr<QueuedRenderer>());

  void BeginFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);
  void DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);
  void EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);

  m2::RectI const OnSize(int w, int h);

  void StartDrag();
  void StopDrag();

  void StartScale();
  void StopScale();

  bool IsEmptyModel() const;

  RenderQueue & GetRenderQueue();

  void SetRenderFn(TRenderFn renderFn);
  void SetEmptyModelFn(TEmptyModelFn const & checkFn);

  bool NeedRedraw() const;
};
