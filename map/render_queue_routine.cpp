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

#include "../indexer/scales.hpp"

#include "events.hpp"
#include "drawer_yg.hpp"
#include "render_queue_routine.hpp"
#include "render_queue.hpp"
#include "tile.hpp"

RenderQueueRoutine::Command::Command(TRenderFn renderFn,
                                     Tiler::RectInfo const & rectInfo,
                                     size_t sequenceID,
                                     TCommandFinishedFn commandFinishedFn)
  : m_renderFn(renderFn),
    m_rectInfo(rectInfo),
    m_sequenceID(sequenceID),
    m_commandFinishedFn(commandFinishedFn)
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

bool RenderQueueRoutine::HasTile(Tiler::RectInfo const & rectInfo)
{
  TileCache & tileCache = m_renderQueue->GetTileCache();
  tileCache.readLock();
  bool res = tileCache.hasTile(rectInfo);
  tileCache.readUnlock();
  return res;
}

void RenderQueueRoutine::AddTile(Tiler::RectInfo const & rectInfo, Tile const & tile)
{
  TileCache & tileCache = m_renderQueue->GetTileCache();
  tileCache.writeLock();
  if (tileCache.hasTile(rectInfo))
    tileCache.touchTile(rectInfo);
  else
    tileCache.addTile(rectInfo, TileCache::Entry(tile, m_resourceManager));
  tileCache.writeUnlock();
}

void RenderQueueRoutine::Cancel()
{
  IRoutine::Cancel();

  threads::MutexGuard guard(m_mutex);

  /// ...Or cancelling the current rendering command in progress.
  if (m_currentCommand != 0)
    m_currentCommand->m_paintEvent->setIsCancelled(true);
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

  params.m_resourceManager = m_resourceManager;
  params.m_frameBuffer = m_frameBuffer;
  params.m_glyphCacheID = m_resourceManager->renderThreadGlyphCacheID(m_threadNum);
  params.m_useOverlay = true;
  params.m_threadID = m_threadNum;
/*  params.m_isDebugging = true;
  params.m_drawPathes = false;
  params.m_drawAreas = false;
  params.m_drawTexts = false; */

  m_threadDrawer = make_shared_ptr(new DrawerYG(m_skinName, params));
  m_threadDrawer->onSize(tileWidth, tileHeight);
  m_threadDrawer->SetVisualScale(m_visualScale);

  ScreenBase frameScreen;
  /// leaving 1px border of fully transparent pixels for sews-free tile blitting.
  m2::RectI renderRect(1, 1, tileWidth - 1, tileWidth - 1);

  frameScreen.OnSize(renderRect);

  while (!IsCancelled())
  {
    {
      threads::MutexGuard guard(m_mutex);

      m_renderQueue->StartCommand();
      m_currentCommand = m_renderQueue->RenderCommands().Front(true);

      if (m_renderQueue->RenderCommands().IsCancelled())
      {
        m_renderQueue->FinishCommand();
        break;
      }

      /// commands from the previous sequence are ignored
      if (m_currentCommand->m_sequenceID < m_renderQueue->CurrentSequence())
        continue;

      if (HasTile(m_currentCommand->m_rectInfo))
        continue;

      m_currentCommand->m_paintEvent = make_shared_ptr(new PaintEvent(m_threadDrawer));
    }

    if (IsCancelled())
    {
      m_renderQueue->FinishCommand();
      break;
    }

    my::Timer timer;

    shared_ptr<yg::gl::BaseTexture> tileTarget;

    tileTarget = m_resourceManager->renderTargets().Front(true);

    if (m_resourceManager->renderTargets().IsCancelled())
    {
      m_renderQueue->FinishCommand();
      break;
    }

    m_threadDrawer->screen()->setRenderTarget(tileTarget);

    shared_ptr<yg::InfoLayer> tileInfoLayer(new yg::InfoLayer());

    m_threadDrawer->screen()->setInfoLayer(tileInfoLayer);

    m_threadDrawer->beginFrame();

    m_threadDrawer->clear(yg::Color(m_bgColor.r, m_bgColor.g, m_bgColor.b, 0));
    m_threadDrawer->screen()->setClipRect(renderRect);
    m_threadDrawer->clear(m_bgColor);

    frameScreen.SetFromRect(m_currentCommand->m_rectInfo.m_rect);

    m2::RectD selectionRect;

    double const inflationSize = 24 * m_visualScale;

    //frameScreen.PtoG(m2::Inflate(m2::RectD(renderRect), inflationSize, inflationSize), selectionRect);
    frameScreen.PtoG(m2::RectD(renderRect), selectionRect);

    m_currentCommand->m_renderFn(
        m_currentCommand->m_paintEvent,
        frameScreen,
        selectionRect,
        m_currentCommand->m_rectInfo.m_drawScale);

    m_threadDrawer->endFrame();
    m_threadDrawer->screen()->resetInfoLayer();

    double duration = timer.ElapsedSeconds();

    TCommandFinishedFn fn;
    Tiler::RectInfo rectInfo;
    Tile tile;

    if (!IsCancelled())
    {
      {
        threads::MutexGuard guard(m_mutex);

        if (m_currentCommand->m_paintEvent->isCancelled())
        {
          m_renderQueue->FinishCommand();
          break;
        }

        rectInfo = m_currentCommand->m_rectInfo;
        tile = Tile(tileTarget, tileInfoLayer, frameScreen, m_currentCommand->m_rectInfo, duration);
        AddTile(rectInfo, tile);

        fn = m_currentCommand->m_commandFinishedFn;

        m_currentCommand.reset();
        m_renderQueue->FinishCommand();
      }

      fn(rectInfo, tile);
    }
    else
      m_renderQueue->FinishCommand();

  }

  // By VNG: We can't destroy render context in drawing thread.
  // Notify render context instead.
  m_renderContext->endThreadDrawing();
}

void RenderQueueRoutine::InitializeGL(shared_ptr<yg::gl::RenderContext> const & renderContext,
                                      shared_ptr<yg::ResourceManager> const & resourceManager,
                                      double visualScale)
{
  m_renderContext = renderContext;
  m_resourceManager = resourceManager;
  m_visualScale = visualScale;
}

void RenderQueueRoutine::MemoryWarning()
{
  m_threadDrawer->screen()->memoryWarning();
}

void RenderQueueRoutine::EnterBackground()
{
  m_threadDrawer->screen()->enterBackground();
}

void RenderQueueRoutine::EnterForeground()
{
  m_threadDrawer->screen()->enterForeground();
}

