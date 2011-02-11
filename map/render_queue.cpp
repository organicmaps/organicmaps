#include "../base/SRC_FIRST.hpp"

#include "render_queue.hpp"

#include "../yg/render_state.hpp"
#include "../yg/rendercontext.hpp"

RenderQueue::RenderQueue(
    string const & skinName,
    bool isMultiSampled,
    bool doPeriodicalUpdate,
    double updateInterval,
    bool isBenchmarking)
  : m_renderState(new yg::gl::RenderState())
{
  m_renderState->m_surfaceWidth = 100;
  m_renderState->m_surfaceHeight = 100;
  m_renderState->m_textureWidth = 256;
  m_renderState->m_textureHeight = 256;
  m_renderState->m_duration = 0;

  m_routine = new RenderQueueRoutine(
      m_renderState,
      skinName,
      isMultiSampled,
      doPeriodicalUpdate,
      updateInterval,
      isBenchmarking);
}

void RenderQueue::initializeGL(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                               shared_ptr<yg::ResourceManager> const & resourceManager,
                               double visualScale)
{
  m_resourceManager = resourceManager;
  m_routine->initializeGL(primaryContext->createShared(),
                          m_resourceManager);
  m_routine->setVisualScale(visualScale);
  m_renderQueueThread.Create(m_routine);
}

RenderQueue::~RenderQueue()
{
  m_renderQueueThread.Cancel();
}

void RenderQueue::AddCommand(RenderQueueRoutine::render_fn_t const & fn, ScreenBase const & frameScreen)
{
  m_routine->addCommand(fn, frameScreen);
}

void RenderQueue::AddBenchmarkCommand(RenderQueueRoutine::render_fn_t const & fn, ScreenBase const & frameScreen)
{
  m_routine->addBenchmarkCommand(fn, frameScreen);
}

void RenderQueue::SetRedrawAll()
{
  m_renderState->m_doRepaintAll = true;
}

void RenderQueue::AddWindowHandle(shared_ptr<WindowHandle> const & windowHandle)
{
  m_routine->addWindowHandle(windowHandle);
}

void RenderQueue::OnSize(size_t w, size_t h)
{
  m_renderState->onSize(w, h);
}

yg::gl::RenderState const RenderQueue::CopyState() const
{
  yg::gl::RenderState state;
  m_renderState->copyTo(state);
  return state;
}

yg::gl::RenderState const & RenderQueue::renderState() const
{
  return *m_renderState.get();
}




