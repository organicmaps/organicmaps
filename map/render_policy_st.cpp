#include "render_policy_st.hpp"
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

RenderPolicyST::RenderPolicyST(VideoTimer * videoTimer,
                               bool useDefaultFB,
                               yg::ResourceManager::Params const & rmParams,
                               shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : QueuedRenderPolicy(1, primaryRC, false),
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
                true,
                0.1,
                false,
                GetPlatform().ScaleEtalonSize(),
                GetPlatform().VisualScale(),
                m_bgColor));

  m_renderQueue->AddWindowHandle(m_windowHandle);

  m_renderQueue->SetGLQueue(GetPacketsQueue(0));
}

void RenderPolicyST::SetRenderFn(TRenderFn renderFn)
{
  QueuedRenderPolicy::SetRenderFn(renderFn);
  m_renderQueue->initializeGL(m_primaryRC, m_resourceManager);
}

void RenderPolicyST::SetEmptyModelFn(TEmptyModelFn const & checkFn)
{
  m_renderQueue->SetEmptyModelFn(checkFn);
}

RenderPolicyST::~RenderPolicyST()
{
  LOG(LINFO, ("destroying RenderPolicyST"));

  base_t::DismissQueuedCommands(0);

  LOG(LINFO, ("shutting down renderQueue"));

  m_renderQueue.reset();

  LOG(LINFO, ("PartialRenderPolicy destroyed"));
}

void RenderPolicyST::DrawFrame(shared_ptr<PaintEvent> const & e,
                               ScreenBase const & s)
{
  base_t::DrawFrame(e, s);

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
    m2::PointD const ptShift = m_renderQueue->renderState().coordSystemShift(false);

    math::Matrix<double, 3, 3> m = m_renderQueue->renderState().m_actualScreen.PtoGMatrix() * s.GtoPMatrix();
    m = math::Shift(m, -ptShift);

    pDrawer->screen()->blit(m_renderQueue->renderState().m_actualTarget, m);
  }
}

void RenderPolicyST::EndFrame(shared_ptr<PaintEvent> const & ev,
                            ScreenBase const & s)
{
  m_renderQueue->renderState().m_mutex->Unlock();
  base_t::EndFrame(ev, s);
}

bool RenderPolicyST::IsEmptyModel() const
{
  return m_renderQueue->renderState().m_isEmptyModelActual;
}

m2::RectI const RenderPolicyST::OnSize(int w, int h)
{
  QueuedRenderPolicy::OnSize(w, h);

  m_renderQueue->OnSize(w, h);

  m2::PointU pt = m_renderQueue->renderState().coordSystemShift();

  return m2::RectI(pt.x, pt.y, pt.x + w, pt.y + h);
}

void RenderPolicyST::StartDrag()
{
  m_DoAddCommand = false;
  base_t::StartDrag();
}

void RenderPolicyST::StopDrag()
{
  m_DoAddCommand = true;
  base_t::StopDrag();
}

void RenderPolicyST::StartScale()
{
  m_DoAddCommand = false;
  base_t::StartScale();
}

void RenderPolicyST::StopScale()
{
  m_DoAddCommand = true;
  base_t::StartScale();
}
