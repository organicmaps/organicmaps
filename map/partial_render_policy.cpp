#include "partial_render_policy.hpp"
#include "events.hpp"
#include "window_handle.hpp"
#include "drawer_yg.hpp"
#include "render_queue.hpp"

#include "../yg/internal/opengl.hpp"
#include "../yg/render_state.hpp"
#include "../yg/base_texture.hpp"

#include "../geometry/transformations.hpp"

#include "../platform/platform.hpp"

#include "../std/bind.hpp"

PartialRenderPolicy::PartialRenderPolicy(VideoTimer * videoTimer,
                                         bool useDefaultFB,
                                         yg::ResourceManager::Params const & rmParams,
                                         shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : RenderPolicy(primaryRC, false),
    m_DoAddCommand(true)
{
  yg::ResourceManager::Params rmp = rmParams;

  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(5000 * sizeof(yg::gl::Vertex),
                                                                       sizeof(yg::gl::Vertex),
                                                                       10000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       4,
                                                                       true,
                                                                       false,
                                                                       2,
                                                                       "primaryStorage");

  rmp.m_smallStoragesParams = yg::ResourceManager::StoragePoolParams(2000 * sizeof(yg::gl::Vertex),
                                                                     sizeof(yg::gl::Vertex),
                                                                     4000 * sizeof(unsigned short),
                                                                     sizeof(unsigned short),
                                                                     4,
                                                                     true,
                                                                     false,
                                                                     1,
                                                                     "smallStorage");

  rmp.m_blitStoragesParams = yg::ResourceManager::StoragePoolParams(10 * sizeof(yg::gl::Vertex),
                                                                    sizeof(yg::gl::Vertex),
                                                                    10 * sizeof(unsigned short),
                                                                    sizeof(unsigned short),
                                                                    50,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "blitStorage");

  rmp.m_guiThreadStoragesParams = yg::ResourceManager::StoragePoolParams(300 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         600 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         20,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "guiThreadStorage");

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                       256,
                                                                       6,
                                                                       rmp.m_texFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture");

  rmp.m_fontTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                    256,
                                                                    6,
                                                                    rmp.m_texFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "fontTextures");

  rmp.m_guiThreadTexturesParams = yg::ResourceManager::TexturePoolParams(256,
                                                                         128,
                                                                         4,
                                                                         rmp.m_texFormat,
                                                                         true,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "guiThreadTexture");

  rmp.m_glyphCacheParams = yg::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 2,
                                                                 1);

  rmp.m_useSingleThreadedOGL = true;
  rmp.m_useVA = !yg::gl::g_isBufferObjectsSupported;

  rmp.fitIntoLimits();

  m_resourceManager.reset(new yg::ResourceManager(rmp));

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  DrawerYG::params_t p;

  p.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(useDefaultFB));
  p.m_resourceManager = m_resourceManager;
  p.m_dynamicPagesCount = 2;
  p.m_textPagesCount = 2;
  p.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  p.m_skinName = GetPlatform().SkinName();
  p.m_visualScale = GetPlatform().VisualScale();
  p.m_isSynchronized = false;
  p.m_useGuiResources = true;

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setVideoTimer(videoTimer);
  m_windowHandle->setRenderContext(primaryRC);

  m_renderQueue.reset(new RenderQueue(GetPlatform().SkinName(),
                false,
                false,
                0.1,
                false,
                GetPlatform().ScaleEtalonSize(),
                GetPlatform().VisualScale(),
                m_bgColor));

  m_renderQueue->AddWindowHandle(m_windowHandle);

  m_glQueue.SetName("glCommands");

  m_renderQueue->SetGLQueue(&m_glQueue, &m_glCondition);
}

void PartialRenderPolicy::SetRenderFn(TRenderFn renderFn)
{
  RenderPolicy::SetRenderFn(renderFn);
  m_renderQueue->initializeGL(m_primaryRC, m_resourceManager);
}

void PartialRenderPolicy::SetEmptyModelFn(TEmptyModelFn const & checkFn)
{
  m_renderQueue->SetEmptyModelFn(checkFn);
}

PartialRenderPolicy::~PartialRenderPolicy()
{
  LOG(LINFO, ("destroying PartialRenderPolicy"));

  {
    threads::ConditionGuard guard(m_glCondition);
    /// unlocking waiting renderThread
    if (!m_glQueue.Empty())
    {
      LOG(LINFO, ("clearing glQueue"));
      m_glQueue.Clear();
      guard.Signal();
    }
  }

  LOG(LINFO, ("shutting down renderQueue"));

  m_renderQueue.reset();

  m_state.reset();
  m_curState.reset();

  LOG(LINFO, ("PartialRenderPolicy destroyed"));
}

void PartialRenderPolicy::ProcessRenderQueue(list<yg::gl::Renderer::Packet> & renderQueue, int maxPackets)
{
  m_frameGLQueue.clear();
  if (maxPackets == -1)
  {
    m_frameGLQueue = renderQueue;
    renderQueue.clear();
  }
  else
  {
    if (renderQueue.empty())
      m_glCondition.Signal();
    else
    {
      /// searching for "frame boundary" markers (empty packets)

      list<yg::gl::Renderer::Packet>::iterator first = renderQueue.begin();
      list<yg::gl::Renderer::Packet>::iterator last = renderQueue.begin();

      int packetsLeft = maxPackets;

      while ((packetsLeft != 0) && (last != renderQueue.end()))
      {
        yg::gl::Renderer::Packet p = *last;
        if ((p.m_command == 0) && (p.m_state == 0))
        {
          LOG(LINFO, ("found frame boundary"));
          /// found frame boundary, copying
          copy(first, last++, back_inserter(m_frameGLQueue));
          /// erasing from the main queue
          renderQueue.erase(first, last);
          first = renderQueue.begin();
          last = renderQueue.begin();
        }
        else
          ++last;

        --packetsLeft;
      }
    }
  }
}

void PartialRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & e,
                                    ScreenBase const & s)
{
  m_resourceManager->mergeFreeResources();

  yg::gl::Screen * screen = e->drawer()->screen().get();

  if (!m_state)
  {
    m_state = screen->createState();
    m_state->m_isDebugging = m_IsDebugging;
  }

  screen->getState(m_state.get());

  m_curState = m_state;

  unsigned cmdProcessed = 0;
  unsigned const maxCmdPerFrame = 10000;

  m_glQueue.ProcessList(bind(&PartialRenderPolicy::ProcessRenderQueue, this, _1, maxCmdPerFrame));

  cmdProcessed = m_frameGLQueue.size();

  for (list<yg::gl::Renderer::Packet>::iterator it = m_frameGLQueue.begin(); it != m_frameGLQueue.end(); ++it)
  {
    if (it->m_state)
    {
      it->m_state->m_isDebugging = m_IsDebugging;
      it->m_state->apply(m_curState.get());
//      OGLCHECK(glFinish());
      m_curState = it->m_state;
    }
    it->m_command->setIsDebugging(m_IsDebugging);
    it->m_command->perform();
//    OGLCHECK(glFinish());
  }

  /// should clear to release resources, refered from the stored commands.
  m_frameGLQueue.clear();

  if (m_IsDebugging)
  {
    LOG(LINFO, ("processed", cmdProcessed, "commands"));
    LOG(LINFO, (m_glQueue.Size(), "commands left"));
  }

  {
    threads::ConditionGuard guard(m_glCondition);
    if (m_glQueue.Empty())
      guard.Signal();
  }

//  OGLCHECK(glFinish());

  m_state->apply(m_curState.get());

//  OGLCHECK(glFinish());

  /// blitting actualTarget

  m_renderQueue->renderStatePtr()->m_doRepaintAll = DoForceUpdate();

  if (m_DoAddCommand && (DoForceUpdate() || (s != m_renderQueue->renderState().m_actualScreen)))
    m_renderQueue->AddCommand(m_renderFn, s);

  SetForceUpdate(false);

  DrawerYG * pDrawer = e->drawer();

  e->drawer()->screen()->clear(m_bgColor);

  m_renderQueue->renderState().m_mutex->Lock();

  if (m_renderQueue->renderState().m_actualTarget.get() != 0)
  {
    if (m_IsDebugging)
      LOG(LINFO, ("actualTarget: ", m_renderQueue->renderState().m_actualTarget->id()));
    m2::PointD const ptShift = m_renderQueue->renderState().coordSystemShift(false);

    math::Matrix<double, 3, 3> m = m_renderQueue->renderState().m_actualScreen.PtoGMatrix() * s.GtoPMatrix();
    m = math::Shift(m, -ptShift);

    pDrawer->screen()->blit(m_renderQueue->renderState().m_actualTarget, m);
  }

  OGLCHECK(glFinish());
}

void PartialRenderPolicy::BeginFrame(shared_ptr<PaintEvent> const & paintEvent,
                                     ScreenBase const & screenBase)
{
  m_IsDebugging = false;
  if (m_IsDebugging)
    LOG(LINFO, ("-------BeginFrame-------"));
}

void PartialRenderPolicy::EndFrame(shared_ptr<PaintEvent> const & paintEvent,
                                   ScreenBase const & screenBase)
{
  m_renderQueue->renderState().m_mutex->Unlock();
  if (m_IsDebugging)
    LOG(LINFO, ("-------EndFrame-------"));
}

bool PartialRenderPolicy::NeedRedraw() const
{
  return RenderPolicy::NeedRedraw() || !m_glQueue.Empty();
}

bool PartialRenderPolicy::IsEmptyModel() const
{
  return m_renderQueue->renderState().m_isEmptyModelActual;
}

m2::RectI const PartialRenderPolicy::OnSize(int w, int h)
{
  RenderPolicy::OnSize(w, h);

  m_renderQueue->OnSize(w, h);

  m2::PointU pt = m_renderQueue->renderState().coordSystemShift();

  return m2::RectI(pt.x, pt.y, pt.x + w, pt.y + h);
}

void PartialRenderPolicy::StartDrag()
{
  m_DoAddCommand = false;
  RenderPolicy::StartDrag();
}

void PartialRenderPolicy::StopDrag()
{
  m_DoAddCommand = true;
  RenderPolicy::StopDrag();
}

void PartialRenderPolicy::StartScale()
{
  m_DoAddCommand = false;
  RenderPolicy::StartScale();
}

void PartialRenderPolicy::StopScale()
{
  m_DoAddCommand = true;
  RenderPolicy::StartScale();
}

RenderQueue & PartialRenderPolicy::GetRenderQueue()
{
  return *m_renderQueue.get();
}
