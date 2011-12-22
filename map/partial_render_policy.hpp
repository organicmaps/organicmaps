#pragma once

#include "render_policy.hpp"
#include "../yg/screen.hpp"
#include "../base/threaded_list.hpp"
#include "../std/scoped_ptr.hpp"

class RenderQueue;
class WindowHandle;

class PartialRenderPolicy : public RenderPolicy
{
private:

  ThreadedList<yg::gl::Renderer::Packet> m_glQueue;
  list<yg::gl::Renderer::Packet> m_frameGLQueue;

  threads::Condition m_glCondition;

  yg::gl::Renderer::Packet m_currentPacket;
  shared_ptr<yg::gl::Screen::BaseState> m_curState;
  bool m_hasPacket;

  shared_ptr<yg::gl::Renderer::BaseState> m_state;

  scoped_ptr<RenderQueue> m_renderQueue;
  bool m_DoAddCommand;
  bool m_DoSynchronize;

  void ProcessRenderQueue(list<yg::gl::Renderer::Packet> & renderQueue, int maxPackets);

  bool m_IsDebugging;

public:

  PartialRenderPolicy(VideoTimer * videoTimer,
                      bool useDefaultFB,
                      yg::ResourceManager::Params const & rmParams,
                      shared_ptr<yg::gl::RenderContext> const & primaryRC);

  ~PartialRenderPolicy();

  void BeginFrame(shared_ptr<PaintEvent> const & paintEvent,
                  ScreenBase const & screenBase);

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent,
                 ScreenBase const & screenBase);

  void EndFrame(shared_ptr<PaintEvent> const & paintEvent,
                ScreenBase const & screenBase);

  m2::RectI const OnSize(int w, int h);

  bool NeedRedraw() const;
  bool IsEmptyModel() const;

  void StartDrag();
  void StopDrag();

  void StartScale();
  void StopScale();

  RenderQueue & GetRenderQueue();

  void SetRenderFn(TRenderFn renderFn);
  void SetEmptyModelFn(TEmptyModelFn const & checkFn);
};
