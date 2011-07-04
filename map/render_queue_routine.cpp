#include "../base/mutex.hpp"
#include "../base/timer.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"

#include "../yg/internal/opengl.hpp"
#include "../yg/render_state.hpp"
#include "../yg/rendercontext.hpp"
#include "../yg/framebuffer.hpp"
#include "../yg/renderbuffer.hpp"
#include "../yg/resource_manager.hpp"
#include "../yg/screen.hpp"
#include "../yg/pen_info.hpp"
#include "../yg/skin.hpp"
#include "../yg/base_texture.hpp"
#include "../yg/info_layer.hpp"
#include "../yg/tile.hpp"

#include "../indexer/scales.hpp"

#include "events.hpp"
#include "drawer_yg.hpp"
#include "window_handle.hpp"
#include "render_queue_routine.hpp"
#include "render_queue.hpp"

RenderQueueRoutine::Command::Command(yg::Tiler::RectInfo const & rectInfo,
                                     render_fn_t renderFn,
                                     size_t seqNum)
  : m_rectInfo(rectInfo),
    m_renderFn(renderFn),
    m_seqNum(seqNum)
{}

RenderQueueRoutine::RenderQueueRoutine(string const & skinName,
                                       bool isBenchmarking,
                                       unsigned scaleEtalonSize,
                                       yg::Color const & bgColor,
                                       size_t threadNum,
                                       RenderQueue * renderQueue)
  : m_threadNum(threadNum),
    m_renderQueue(renderQueue)
{
  m_skinName = skinName;
  m_visualScale = 0;
  m_isBenchmarking = isBenchmarking;
  m_scaleEtalonSize = scaleEtalonSize;
  m_bgColor = bgColor;
}

void RenderQueueRoutine::Cancel()
{
  IRoutine::Cancel();
  /// ...Or cancelling the current rendering command in progress.
  if (m_currentCommand != 0)
    m_currentCommand->m_paintEvent->setIsCancelled(true);
}

void RenderQueueRoutine::setVisualScale(double visualScale)
{
  m_visualScale = visualScale;
}

void RenderQueueRoutine::Do()
{
  m_renderContext->makeCurrent();

  m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer());

  unsigned tileWidth = m_resourceManager->tileTextureWidth();
  unsigned tileHeight = m_resourceManager->tileTextureHeight();

  shared_ptr<yg::gl::RenderBuffer> depthBuffer(new yg::gl::RenderBuffer(tileWidth, tileHeight, true));
  m_frameBuffer->setDepthBuffer(depthBuffer);

  DrawerYG::params_t params;

  shared_ptr<yg::InfoLayer> infoLayer(new yg::InfoLayer());

  params.m_resourceManager = m_resourceManager;
  params.m_frameBuffer = m_frameBuffer;
  params.m_infoLayer = infoLayer;
  params.m_glyphCacheID = m_resourceManager->renderThreadGlyphCacheID(m_threadNum);
  params.m_useOverlay = true;
  params.m_threadID = m_threadNum;
/*  params.m_isDebugging = true;
  params.m_drawPathes = false;
  params.m_drawAreas = false;
  params.m_drawTexts = false; */

  m_threadDrawer = make_shared_ptr(new DrawerYG(m_skinName, params));
  m_threadDrawer->onSize(tileWidth, tileHeight);

  CHECK(m_visualScale != 0, ("VisualScale is not set"));
  m_threadDrawer->SetVisualScale(m_visualScale);

  ScreenBase frameScreen;
  frameScreen.OnSize(0, 0, tileWidth, tileHeight);

  while (!IsCancelled())
  {
    {
      threads::MutexGuard guard(m_mutex);

      m_currentCommand = m_renderQueue->RenderCommands().Front(true);

      if (m_renderQueue->RenderCommands().IsCancelled())
      {
        LOG(LINFO, (m_threadNum, " cancelled on renderCommands"));
        break;
      }

      m_currentCommand->m_paintEvent = make_shared_ptr(new PaintEvent(m_threadDrawer));

      /// commands from the previous sequence are ignored
      if (m_currentCommand->m_seqNum < m_renderQueue->CurrentSequence())
        continue;

      bool hasTile = false;

      m_renderQueue->TileCache().lock();
      hasTile = m_renderQueue->TileCache().hasTile(m_currentCommand->m_rectInfo);
      m_renderQueue->TileCache().unlock();

      if (hasTile)
        continue;
    }

    if (IsCancelled())
      break;

    my::Timer timer;

    shared_ptr<yg::gl::BaseTexture> tileTarget;

    tileTarget = m_resourceManager->renderTargets().Front(true);

    if (m_resourceManager->renderTargets().IsCancelled())
    {
      LOG(LINFO, (m_threadNum, " cancelled on renderTargets"));
      break;
    }

    m_threadDrawer->screen()->setRenderTarget(tileTarget);

    m_threadDrawer->beginFrame();

    m_threadDrawer->clear(m_bgColor);

    frameScreen.SetFromRect(m_currentCommand->m_rectInfo.m_rect);

    m_currentCommand->m_renderFn(
        m_currentCommand->m_paintEvent,
        frameScreen,
        m_currentCommand->m_rectInfo.m_rect,
        m_currentCommand->m_rectInfo.m_drawScale);

    /// rendering all collected texts
//      m_infoLayer->draw(m_threadDrawer->screen().get(), math::Identity<double, 3>());

    m_threadDrawer->endFrame();

    double duration = timer.ElapsedSeconds();

    if (!IsCancelled())
    {
      /// We shouldn't update the actual target as it's already happened (through callback function)
      /// in the endFrame function
      /// updateActualTarget();

      {
        threads::MutexGuard guard(m_mutex);

        if (!m_currentCommand->m_paintEvent->isCancelled())
        {
          yg::Tile tile(tileTarget, frameScreen, m_currentCommand->m_rectInfo, duration);
          m_renderQueue->TileCache().lock();
          m_renderQueue->TileCache().addTile(m_currentCommand->m_rectInfo, yg::TileCache::Entry(tile, m_resourceManager));
          m_renderQueue->TileCache().unlock();
        }

        m_currentCommand.reset();
      }

      invalidate();

      callRenderCommandFinishedFns();
    }

  }

  // By VNG: We can't destroy render context in drawing thread.
  // Notify render context instead.
  m_renderContext->endThreadDrawing();
}

void RenderQueueRoutine::addWindowHandle(shared_ptr<WindowHandle> window)
{
  m_windowHandles.push_back(window);
}

void RenderQueueRoutine::invalidate()
{
  for_each(m_windowHandles.begin(),
           m_windowHandles.end(),
           bind(&WindowHandle::invalidate, _1));
}

void RenderQueueRoutine::initializeGL(shared_ptr<yg::gl::RenderContext> const & renderContext,
                                      shared_ptr<yg::ResourceManager> const & resourceManager)
{
  m_renderContext = renderContext;
  m_resourceManager = resourceManager;
}

void RenderQueueRoutine::memoryWarning()
{
  m_threadDrawer->screen()->memoryWarning();
}

void RenderQueueRoutine::enterBackground()
{
  m_threadDrawer->screen()->enterBackground();
}

void RenderQueueRoutine::enterForeground()
{
  m_threadDrawer->screen()->enterForeground();
}

void RenderQueueRoutine::addRenderCommandFinishedFn(renderCommandFinishedFn fn)
{
  m_renderCommandFinishedFns.push_back(fn);
}

void RenderQueueRoutine::callRenderCommandFinishedFns()
{
  if (!m_renderCommandFinishedFns.empty())
    for (list<renderCommandFinishedFn>::const_iterator it = m_renderCommandFinishedFns.begin(); it != m_renderCommandFinishedFns.end(); ++it)
      (*it)();
}
