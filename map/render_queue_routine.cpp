#include "../base/SRC_FIRST.hpp"
#include "../base/mutex.hpp"
#include "../base/timer.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"

#include "../yg/internal/opengl.hpp"
#include "../yg/render_state.hpp"
#include "../yg/rendercontext.hpp"
#include "../yg/framebuffer.hpp"
#include "../yg/renderbuffer.hpp"
#include "../yg/texture.hpp"
#include "../yg/resource_manager.hpp"
#include "../yg/screen.hpp"
#include "../yg/pen_info.hpp"
#include "../yg/skin.hpp"

#include "../indexer/scales.hpp"

#include "events.hpp"
#include "drawer_yg.hpp"
#include "window_handle.hpp"
#include "render_queue_routine.hpp"
RenderQueueRoutine::RenderModelCommand::RenderModelCommand(ScreenBase const & frameScreen,
                                                             render_fn_t renderFn)
                                                               : m_frameScreen(frameScreen),
                                                               m_renderFn(renderFn)
{}

RenderQueueRoutine::RenderQueueRoutine(shared_ptr<yg::gl::RenderState> const & renderState,
                                       string const & skinName,
                                       bool isMultiSampled,
                                       bool doPeriodicalUpdate,
                                       double updateInterval,
                                       bool isBenchmarking,
                                       unsigned scaleEtalonSize,
                                       double visualScale,
                                       yg::Color const & bgColor)
{
  m_skinName = skinName;
  m_visualScale = visualScale;
  m_renderState = renderState;
  m_renderState->addInvalidateFn(bind(&RenderQueueRoutine::invalidate, this));
  m_isMultiSampled = isMultiSampled;
  m_doPeriodicalUpdate = doPeriodicalUpdate;
  m_updateInterval = updateInterval;
  m_isBenchmarking = isBenchmarking;
  m_scaleEtalonSize = scaleEtalonSize;
  m_bgColor = bgColor;
  m_glQueue = 0;
}

void RenderQueueRoutine::Cancel()
{
  IRoutine::Cancel();

  {
    threads::ConditionGuard guard(m_hasRenderCommands);

    if (m_currentRenderCommand != 0)
    {
      LOG(LDEBUG, ("cancelling current renderCommand in progress"));
      m_currentRenderCommand->m_paintEvent->cancel();
    }

    LOG(LDEBUG, ("waking up the sleeping thread..."));

    m_hasRenderCommands.Signal();
  }
}

void RenderQueueRoutine::onSize(int w, int h)
{
  size_t texW = m_renderState->m_textureWidth;
  size_t texH = m_renderState->m_textureHeight;

  m_newDepthBuffer.reset();
  m_newDepthBuffer.reset(new yg::gl::RenderBuffer(texW, texH, true));
  m_newActualTarget = m_resourceManager->createRenderTarget(texW, texH);
  m_newBackBuffer = m_resourceManager->createRenderTarget(texW, texH);
}

void RenderQueueRoutine::SetCountryNameFn(TCountryNameFn countryNameFn)
{
  m_countryNameFn = countryNameFn;
}

void RenderQueueRoutine::processResize(ScreenBase const & frameScreen)
{
  if (m_renderState->m_isResized)
  {
    size_t texW = m_renderState->m_textureWidth;
    size_t texH = m_renderState->m_textureHeight;

    m_threadDrawer->onSize(texW, texH);

    m_renderState->m_depthBuffer = m_newDepthBuffer;
    m_threadDrawer->screen()->setDepthBuffer(m_renderState->m_depthBuffer);
    m_newDepthBuffer.reset();

    shared_ptr<yg::gl::BaseTexture> oldActualTarget = m_renderState->m_actualTarget;

    m_renderState->m_actualTarget.reset();
    m_renderState->m_actualTarget = m_newActualTarget;
    m_newActualTarget.reset();

    m_auxScreen->onSize(texW, texH);
    m_auxScreen->setRenderTarget(m_renderState->m_actualTarget);
    m_auxScreen->beginFrame();
    m_auxScreen->clear(m_bgColor);

    if (oldActualTarget != 0)
    {
      m_auxScreen->blit(oldActualTarget,
                        m_renderState->m_actualScreen,
                        frameScreen);
      oldActualTarget.reset();
    }
    m_auxScreen->endFrame();

    shared_ptr<yg::gl::BaseTexture> oldBackBuffer = m_renderState->m_backBuffer;
    m_renderState->m_backBuffer.reset();
    m_renderState->m_backBuffer = m_newBackBuffer;
    m_newBackBuffer.reset();
    m_auxScreen->setRenderTarget(m_renderState->m_backBuffer);
    m_auxScreen->beginFrame();
    m_auxScreen->clear(m_bgColor);

    if (oldBackBuffer != 0)
    {
      m_auxScreen->blit(oldBackBuffer,
                        m_renderState->m_actualScreen,
                        frameScreen);
      oldBackBuffer.reset();
    }
    m_auxScreen->endFrame();

    /// TODO : make as a command
    m_renderState->m_actualScreen = frameScreen;

    m_renderState->m_isResized = false;
  }
}

bool RenderQueueRoutine::getUpdateAreas(
    ScreenBase const & oldScreen,
    m2::RectI const & oldRect,
    ScreenBase const & newScreen,
    m2::RectI const & newRect,
    vector<m2::RectI> & areas)
{
  areas.clear();

  if (IsPanningAndRotate(oldScreen, newScreen))
  {
    m2::RectD o(newScreen.GtoP(oldScreen.PtoG(m2::PointD(oldRect.minX(), oldRect.minY()))),
                 newScreen.GtoP(oldScreen.PtoG(m2::PointD(oldRect.maxX(), oldRect.maxY()))));
    m2::RectD n(newRect);

    /// checking two corner cases
    if (o.IsRectInside(n))
      return false;
    if (!o.IsIntersect(n))
    {
      areas.push_back(newRect);
      return true;
    }

    double leftBarMinX = 0;
    double leftBarMaxX = 0;
    double rightBarMinX = n.maxX();
    double rightBarMaxX = n.maxX();
    double topBarMinY = 0;
    double topBarMaxY = 0;
    double bottomBarMinY = n.maxY();
    double bottomBarMaxY = n.maxY();

    if (o.minX() > n.minX())
      leftBarMaxX = ceil(o.minX());
    if (o.maxX() < n.maxX())
      rightBarMinX = floor(o.maxX());
    if (o.minY() > n.minY())
      topBarMaxY = ceil(o.minY());
    if (o.maxY() < n.maxY())
      bottomBarMinY = floor(o.maxY());

    if ((int)leftBarMinX != (int)leftBarMaxX)
      areas.push_back(m2::RectI((int)leftBarMinX, (int)topBarMinY, (int)leftBarMaxX, (int)bottomBarMinY));
    if ((int)topBarMinY != (int)topBarMaxY)
      areas.push_back(m2::RectI((int)leftBarMaxX, (int)topBarMinY, (int)rightBarMaxX, (int)topBarMaxY));
    if ((int)rightBarMinX != (int)rightBarMaxX)
      areas.push_back(m2::RectI((int)rightBarMinX, (int)topBarMaxY, (int)rightBarMaxX, (int)bottomBarMaxY));
    if ((int)bottomBarMinY != (int)bottomBarMaxY)
      areas.push_back(m2::RectI((int)leftBarMinX, (int)bottomBarMinY, (int)rightBarMinX, (int)bottomBarMaxY));

    return false;
  }
  else
  {
    if (m_doPeriodicalUpdate)
      areas.push_back(newRect);
    else
    {
      int sx = ( newRect.SizeX() + 4 ) / 5;
      int sy = ( newRect.SizeY() + 4 ) / 5;
      m2::RectI r(0, 0, sx, sy);

      areas.push_back(m2::Offset(r, 2 * sx, 2 * sy));
      areas.push_back(m2::Offset(r, 2 * sx, 1 * sy));
      areas.push_back(m2::Offset(r, 3 * sx, 1 * sy));
      areas.push_back(m2::Offset(r, 3 * sx, 2 * sy));
      areas.push_back(m2::Offset(r, 3 * sx, 3 * sy));
      areas.push_back(m2::Offset(r, 2 * sx, 3 * sy));
      areas.push_back(m2::Offset(r, 1 * sx, 3 * sy));
      areas.push_back(m2::Offset(r, 1 * sx, 2 * sy));
      areas.push_back(m2::Offset(r, 1 * sx, 1 * sy));
      areas.push_back(m2::Offset(r, 1 * sx, 0 * sy));
      areas.push_back(m2::Offset(r, 1 * sx, 0 * sy));
      areas.push_back(m2::Offset(r, 2 * sx, 0 * sy));
      areas.push_back(m2::Offset(r, 3 * sx, 0 * sy));
      areas.push_back(m2::Offset(r, 4 * sx, 0 * sy));
      areas.push_back(m2::Offset(r, 4 * sx, 1 * sy));
      areas.push_back(m2::Offset(r, 4 * sx, 2 * sy));
      areas.push_back(m2::Offset(r, 4 * sx, 3 * sy));
      areas.push_back(m2::Offset(r, 4 * sx, 4 * sy));
      areas.push_back(m2::Offset(r, 3 * sx, 4 * sy));
      areas.push_back(m2::Offset(r, 2 * sx, 4 * sy));
      areas.push_back(m2::Offset(r, 1 * sx, 4 * sy));
      areas.push_back(m2::Offset(r, 0 * sx, 4 * sy));
      areas.push_back(m2::Offset(r, 0 * sx, 3 * sy));
      areas.push_back(m2::Offset(r, 0 * sx, 2 * sy));
      areas.push_back(m2::Offset(r, 0 * sx, 1 * sy));
      areas.push_back(m2::Offset(r, 0 * sx, 0 * sy));
    }
    return true;
  }
}

void RenderQueueRoutine::setVisualScale(double visualScale)
{
  m_visualScale = visualScale;
}

void RenderQueueRoutine::waitForRenderCommand(list<shared_ptr<RenderModelCommand> > & cmdList,
                                              threads::ConditionGuard & guard)
{
  while (cmdList.empty())
  {
    guard.Wait();
    if (IsCancelled())
      break;
  }
}

void RenderQueueRoutine::Do()
{
  if (!m_glQueue)
    m_renderContext->makeCurrent();

  DrawerYG::params_t params;

  params.m_resourceManager = m_resourceManager;
  params.m_frameBuffer = m_frameBuffer;
  params.m_renderState = m_renderState;
  params.m_doPeriodicalUpdate = m_doPeriodicalUpdate;
  params.m_auxFrameBuffer = m_auxFrameBuffer;
  params.m_updateInterval = m_updateInterval;
  params.m_skinName = m_skinName;
  params.m_visualScale = m_visualScale;
  params.m_threadID = 0;
  params.m_glyphCacheID = m_resourceManager->renderThreadGlyphCacheID(0);
  params.m_renderQueue = m_glQueue;
/*  params.m_isDebugging = true;
  params.m_drawPathes = false;
  params.m_drawAreas = false;
  params.m_drawTexts = false;
  params.m_drawSymbols = false;*/

  m_threadDrawer = make_shared_ptr(new DrawerYG(params));

  yg::gl::Screen::Params auxParams;
  auxParams.m_frameBuffer = m_auxFrameBuffer;
  auxParams.m_resourceManager = m_resourceManager;
  auxParams.m_renderQueue = m_glQueue;

  m_auxScreen = make_shared_ptr(new yg::gl::Screen(auxParams));

  bool isPanning = false;
  bool doRedrawAll = false;
  bool fullRectRepaint = false;
  /// update areas in pixel coordinates.
  vector<m2::RectI> areas;

  m2::RectI  surfaceRect;
  m2::RectI  textureRect;

  shared_ptr<yg::Overlay> overlay(new yg::Overlay());
  overlay->setCouldOverlap(false);
  m_threadDrawer->screen()->setOverlay(overlay);

  while (!IsCancelled())
  {
    {
      threads::ConditionGuard guard(m_hasRenderCommands);

      waitForRenderCommand(m_renderCommands, guard);

      if (IsCancelled())
      {
        LOG(LDEBUG, ("thread is cancelled while waiting for a render command"));
        break;
      }

      m_currentRenderCommand = m_renderCommands.front();
      m_renderCommands.erase(m_renderCommands.begin());

      m_currentRenderCommand->m_paintEvent = make_shared_ptr(new PaintEvent(m_threadDrawer.get()));

      /// this prevents the framework from flooding us with a bunch of RenderCommands with the same screen.
      {
        threads::MutexGuard guard(*m_renderState->m_mutex.get());

        m_renderState->m_currentScreen = m_currentRenderCommand->m_frameScreen;

        m2::RectI prevRect(0, 0, 0, 0);
        ScreenBase prevScreen = m_renderState->m_actualScreen;

        if (m_renderState->m_actualTarget != 0)
          prevRect = m2::RectI(0, 0,
                               m_renderState->m_actualTarget->width(),
                               m_renderState->m_actualTarget->height());

        processResize(m_currentRenderCommand->m_frameScreen);

        /// saving parameters, which might be changed from the GUI thread for later use.
        surfaceRect = m2::RectI(0, 0, m_renderState->m_surfaceWidth, m_renderState->m_surfaceHeight);
        textureRect = m2::RectI(0, 0, m_renderState->m_textureWidth, m_renderState->m_textureHeight);

        m2::RectI curRect = textureRect;

        doRedrawAll = m_renderState->m_doRepaintAll;

        fullRectRepaint = false;

        if (m_renderState->m_doRepaintAll)
        {
          areas.clear();
          areas.push_back(curRect);
          fullRectRepaint = true;
          m_threadDrawer->screen()->overlay()->clear();
          m_renderState->m_doRepaintAll = false;
        }
        else
        {
          fullRectRepaint = getUpdateAreas(prevScreen,
                                           prevRect,
                                           m_currentRenderCommand->m_frameScreen,
                                           curRect,
                                           areas);
/*          if ((areas.size() == 1) && (areas[0] == curRect))
            fullRectRepaint = true;*/
        }

        isPanning = IsPanningAndRotate(prevScreen, m_renderState->m_currentScreen);

        if (isPanning)
        {
          math::Matrix<double, 3, 3> offsetM = prevScreen.PtoGMatrix() * m_renderState->m_currentScreen.GtoPMatrix();
          m2::RectD oldRect = m2::RectD(m2::PointD(prevRect.minX(), prevRect.minY()) * offsetM,
                                        m2::PointD(prevRect.maxX(), prevRect.maxY()) * offsetM);
          m2::RectD redrawTextRect(curRect);

          if (!redrawTextRect.Intersect(oldRect))
            redrawTextRect = m2::RectD(0, 0, 0, 0);

          m_threadDrawer->screen()->overlay()->offset(
                m2::PointD(0, 0) * offsetM,
                redrawTextRect
                );
        }
        else
        {
          m_threadDrawer->screen()->overlay()->clear();
          m_renderState->m_isEmptyModelCurrent = true;
        }
      }
    }

    my::Timer timer;

    /// At this point renderQueue->mutex is released to allow
    /// main thread to add new rendering tasks and blit already
    /// rendered model while the current command is actually rendering.

    if (IsCancelled())
    {
      LOG(LDEBUG, ("cancelled before processing currentRenderCommand"));
    }
    else if (m_currentRenderCommand != 0)
    {
      /// this fixes some strange issue with multisampled framebuffer.
      /// setRenderTarget should be made here.
      m_threadDrawer->screen()->setRenderTarget(m_renderState->m_backBuffer);

      m_threadDrawer->beginFrame();

      m_threadDrawer->screen()->enableClipRect(true);
      m_threadDrawer->screen()->setClipRect(textureRect);
      m_threadDrawer->clear(m_bgColor);

      if ((isPanning) && (!doRedrawAll))
      {
        m_threadDrawer->screen()->blit(
            m_renderState->m_actualTarget,
            m_renderState->m_actualScreen,
            m_renderState->m_currentScreen);
      }

      m_threadDrawer->endFrame();

      //m_threadDrawer->screen()->setNeedTextRedraw(isPanning);

      ScreenBase const & frameScreen = m_currentRenderCommand->m_frameScreen;
      m2::RectD glbRect;
      frameScreen.PtoG(m2::RectD(m2::RectD(textureRect).Center() - m2::PointD(m_scaleEtalonSize / 2, m_scaleEtalonSize / 2),
                                 m2::RectD(textureRect).Center() + m2::PointD(m_scaleEtalonSize / 2, m_scaleEtalonSize / 2)),
                       glbRect);
//      frameScreen.PtoG(m2::RectD(surfaceRect), glbRect);
      int scaleLevel = scales::GetScaleLevel(glbRect);

      bool cumulativeEmptyModelCurrent = true;

      for (size_t i = 0; i < areas.size(); ++i)
      {
        if ((areas[i].SizeX() != 0) && (areas[i].SizeY() != 0))
        {
          frameScreen.PtoG(m2::Inflate<double>(m2::RectD(areas[i]), 30 * m_visualScale, 30 * m_visualScale), glbRect);
          if ((glbRect.SizeX() == 0) || (glbRect.SizeY() == 0))
            continue;

          m_threadDrawer->screen()->setClipRect(areas[i]);
          m_threadDrawer->screen()->enableClipRect(true);
          m_threadDrawer->screen()->beginFrame();

          m_currentRenderCommand->m_renderFn(
              m_currentRenderCommand->m_paintEvent,
              m_currentRenderCommand->m_frameScreen,
              glbRect,
              glbRect,
              scaleLevel,
              false);

          /// all unprocessed commands should be cancelled
          if (m_currentRenderCommand->m_paintEvent->isCancelled() && m_glQueue)
            m_glQueue->cancelCommands();

          if (!m_renderState->m_isEmptyModelCurrent)
            cumulativeEmptyModelCurrent = m_renderState->m_isEmptyModelCurrent;

          m_threadDrawer->screen()->endFrame();

          if (IsCancelled())
            break;
        }
      }

      if (IsCancelled())
        break;

      /// if something were actually drawn, or (exclusive or) we are repainting the whole rect
      if (!cumulativeEmptyModelCurrent || fullRectRepaint)
      {
        m2::PointD centerPt = m_currentRenderCommand->m_frameScreen.GlobalRect().GetGlobalRect().Center();
        string countryName = m_countryNameFn(centerPt);
        m_renderState->m_countryNameActual = countryName;
        m_renderState->m_isEmptyModelActual = cumulativeEmptyModelCurrent && !countryName.empty();
      }

      /// setting the "whole texture" clip rect to render texts opened by panning.
      m_threadDrawer->screen()->setClipRect(textureRect);

      m_threadDrawer->beginFrame();

      m_threadDrawer->screen()->overlay()->draw(m_threadDrawer->screen().get(), math::Identity<double, 3>());

      m_threadDrawer->endFrame();
    }

    double duration = timer.ElapsedSeconds();

    if (!IsCancelled())
    {
      {
        threads::ConditionGuard g1(m_hasRenderCommands);

        {
          threads::MutexGuard g2(*m_renderState->m_mutex.get());
          m_renderState->m_duration = duration;
        }

        m_currentRenderCommand.reset();

        if (m_renderCommands.empty())
          g1.Signal();
      }

      invalidate();
    }
  }

  // By VNG: We can't destroy render context in drawing thread.
  // Notify render context instead.
  if (!m_glQueue)
    m_renderContext->endThreadDrawing();
}

void RenderQueueRoutine::addWindowHandle(shared_ptr<WindowHandle> window)
{
  m_windowHandles.push_back(window);
}

void RenderQueueRoutine::Invalidate::perform()
{
  if (isDebugging())
    LOG(LDEBUG, ("performing Invalidate command"));
  for_each(m_windowHandles.begin(),
           m_windowHandles.end(),
           bind(&WindowHandle::invalidate, _1));
}

void RenderQueueRoutine::Invalidate::cancel()
{
  perform();
}

void RenderQueueRoutine::invalidate()
{
  for_each(m_windowHandles.begin(),
           m_windowHandles.end(),
           bind(&WindowHandle::invalidate, _1));

  if (m_glQueue)
  {
    shared_ptr<Invalidate> command(new Invalidate());
    command->m_windowHandles = m_windowHandles;
    m_glQueue->processPacket(yg::gl::Packet(command, yg::gl::Packet::ECheckPoint));
  }
}

void RenderQueueRoutine::addCommand(render_fn_t const & fn, ScreenBase const & frameScreen)
{
  /// Command queue modification is syncronized by mutex
  threads::ConditionGuard guard(m_hasRenderCommands);

  bool needToSignal = m_renderCommands.empty();

  /// Clearing a list of commands.
  m_renderCommands.clear();

  /// pushing back new RenderCommand.
  m_renderCommands.push_back(make_shared_ptr(new RenderModelCommand(frameScreen, fn)));

  /// if we are in benchmarking mode, we shouldn't cancel any render command
  /// else, if we are not panning, we should cancel the render command in progress to start a new one
  if ((m_currentRenderCommand != 0)
   && (!IsPanningAndRotate(m_currentRenderCommand->m_frameScreen, frameScreen)))
    m_currentRenderCommand->m_paintEvent->cancel();

  if (needToSignal)
    guard.Signal();
}

void RenderQueueRoutine::initializeGL(shared_ptr<yg::gl::RenderContext> const & renderContext,
                                      shared_ptr<yg::ResourceManager> const & resourceManager)
{
  m_frameBuffer.reset();
  m_frameBuffer.reset(new yg::gl::FrameBuffer());
  m_auxFrameBuffer.reset();
  m_auxFrameBuffer.reset(new yg::gl::FrameBuffer());
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

void RenderQueueRoutine::waitForEmptyAndFinished()
{
  /// Command queue modification is syncronized by mutex
  threads::ConditionGuard guard(m_hasRenderCommands);

  if (!m_renderCommands.empty() || (m_currentRenderCommand != 0))
    guard.Wait();
}

void RenderQueueRoutine::setGLQueue(yg::gl::PacketsQueue * glQueue)
{
  m_glQueue = glQueue;
}
