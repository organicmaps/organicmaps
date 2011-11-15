#include "partial_render_policy.hpp"
#include "events.hpp"
#include "window_handle.hpp"
#include "drawer_yg.hpp"
#include "render_queue.hpp"

#include "../yg/internal/opengl.hpp"
#include "../yg/render_state.hpp"

#include "../geometry/transformations.hpp"

#include "../platform/platform.hpp"

#include "../std/bind.hpp"

PartialRenderPolicy::PartialRenderPolicy(VideoTimer * videoTimer,
                                         DrawerYG::Params const & params,
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
                                                                       15,
                                                                       false,
                                                                       1,
                                                                       "primaryStorage");

  rmp.m_smallStoragesParams = yg::ResourceManager::StoragePoolParams(2000 * sizeof(yg::gl::Vertex),
                                                                     sizeof(yg::gl::Vertex),
                                                                     4000 * sizeof(unsigned short),
                                                                     sizeof(unsigned short),
                                                                     100,
                                                                     false,
                                                                     1,
                                                                     "smallStorage");

  rmp.m_blitStoragesParams = yg::ResourceManager::StoragePoolParams(10 * sizeof(yg::gl::AuxVertex),
                                                                    sizeof(yg::gl::AuxVertex),
                                                                    10 * sizeof(unsigned short),
                                                                    sizeof(unsigned short),
                                                                    50,
                                                                    true,
                                                                    1,
                                                                    "blitStorage");

  rmp.m_tinyStoragesParams = yg::ResourceManager::StoragePoolParams(300 * sizeof(yg::gl::Vertex),
                                                                    sizeof(yg::gl::Vertex),
                                                                    600 * sizeof(unsigned short),
                                                                    sizeof(unsigned short),
                                                                    20,
                                                                    true,
                                                                    1,
                                                                    "tinyStorage");

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                       256,
                                                                       10,
                                                                       rmp.m_rtFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture");

  rmp.m_fontTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                    256,
                                                                    5,
                                                                    rmp.m_rtFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "fontTextures");

  rmp.m_glyphCacheParams = yg::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 2,
                                                                 1);

  rmp.m_useSingleThreadedOGL = true;
  rmp.m_useVA = !yg::gl::g_isBufferObjectsSupported;

  m_resourceManager.reset(new yg::ResourceManager(rmp));

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  DrawerYG::params_t p = params;

  p.m_resourceManager = m_resourceManager;
  p.m_dynamicPagesCount = 2;
  p.m_textPagesCount = 2;
  p.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  p.m_skinName = GetPlatform().SkinName();
  p.m_visualScale = GetPlatform().VisualScale();
  p.m_isSynchronized = false;
  p.m_useTinyStorage = true;

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setVideoTimer(videoTimer);
  m_windowHandle->setRenderContext(primaryRC);

  m_renderQueue.reset(new RenderQueue(GetPlatform().SkinName(),
                false,
                true,
                0.1,
                false,
                GetPlatform().ScaleEtalonSize(),
                GetPlatform().VisualScale(),
                m_bgColor));

  m_renderQueue->AddWindowHandle(m_windowHandle);

  m_renderQueue->SetGLQueue(&m_glQueue, &m_glCondition);
}

void PartialRenderPolicy::SetRenderFn(TRenderFn renderFn)
{
  RenderPolicy::SetRenderFn(renderFn);
  m_renderQueue->initializeGL(m_primaryRC, m_resourceManager);
}

void PartialRenderPolicy::ProcessRenderQueue(list<yg::gl::Renderer::Packet> & renderQueue)
{
  if (renderQueue.empty())
  {
    m_hasPacket = false;
    m_glCondition.Signal();
  }
  else
  {
    m_hasPacket = true;
    m_currentPacket = renderQueue.front();
    renderQueue.pop_front();
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
    m_state->m_isDebugging = true;
  }

  screen->getState(m_state.get());

  m_curState = m_state;

  unsigned cmdProcessed = 0;
  unsigned const maxCmdPerFrame = 10;

  while (true)
  {
    m_glQueue.ProcessList(bind(&PartialRenderPolicy::ProcessRenderQueue, this, _1));

    if ((m_hasPacket) && (cmdProcessed < maxCmdPerFrame))
    {
      cmdProcessed++;
      if (m_currentPacket.m_state)
      {
        m_currentPacket.m_state->apply(m_curState.get());
        m_curState = m_currentPacket.m_state;
      }
      m_currentPacket.m_command->perform();
    }
    else
      break;
  }

  /// should we continue drawing commands on the next frame
  if ((cmdProcessed == maxCmdPerFrame) && m_hasPacket)
  {
    LOG(LINFO, ("will continue on the next frame(", cmdProcessed, ")"));
  }
  else
  {
    if (cmdProcessed != 0)
      LOG(LINFO, ("finished sequence of commands(", cmdProcessed, ")"));
  }

  {
    threads::ConditionGuard guard(m_glCondition);
    if (m_glQueue.Empty())
      guard.Signal();
  }

  OGLCHECK(glFinish());

  m_state->apply(m_curState.get());

  OGLCHECK(glFinish());

  /// blitting actualTarget

  if (m_DoAddCommand && (s != m_renderQueue->renderState().m_actualScreen))
    m_renderQueue->AddCommand(m_renderFn, s);

  DrawerYG * pDrawer = e->drawer();

  e->drawer()->screen()->clear(m_bgColor);

  m_renderQueue->renderState().m_mutex->Lock();

  if (m_renderQueue->renderState().m_actualTarget.get() != 0)
  {
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
  LOG(LINFO, ("-------BeginFrame-------"));
}

void PartialRenderPolicy::EndFrame(shared_ptr<PaintEvent> const & paintEvent,
                                   ScreenBase const & screenBase)
{
  m_renderQueue->renderState().m_mutex->Unlock();
  LOG(LINFO, ("-------EndFrame-------"));
}

bool PartialRenderPolicy::NeedRedraw() const
{
  return RenderPolicy::NeedRedraw() || !m_glQueue.Empty();
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
