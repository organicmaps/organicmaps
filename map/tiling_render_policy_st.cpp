#include "../base/SRC_FIRST.hpp"

#include "tiling_render_policy_st.hpp"

#include "drawer_yg.hpp"
#include "window_handle.hpp"
#include "events.hpp"

#include "../yg/framebuffer.hpp"
#include "../yg/renderbuffer.hpp"
#include "../yg/resource_manager.hpp"
#include "../yg/base_texture.hpp"
#include "../yg/internal/opengl.hpp"

#include "../platform/platform.hpp"

#include "../base/SRC_FIRST.hpp"

#include "../platform/platform.hpp"
#include "../std/bind.hpp"

#include "../geometry/screenbase.hpp"

#include "../yg/base_texture.hpp"
#include "../yg/internal/opengl.hpp"

#include "drawer_yg.hpp"
#include "events.hpp"
#include "tiling_render_policy_mt.hpp"
#include "window_handle.hpp"
#include "screen_coverage.hpp"

TilingRenderPolicyST::TilingRenderPolicyST(VideoTimer * videoTimer,
                                           bool useDefaultFB,
                                           yg::ResourceManager::Params const & rmParams,
                                           shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : RenderPolicy(primaryRC, true)
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
                                                                     100,
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

  rmp.m_multiBlitStoragesParams = yg::ResourceManager::StoragePoolParams(1500 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         3000 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         10,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "multiBlitStorage");

  rmp.m_guiThreadStoragesParams = yg::ResourceManager::StoragePoolParams(300 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         600 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         200,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "guiThreadStorage");

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                       256,
                                                                       10,
                                                                       rmp.m_texFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture");

  rmp.m_fontTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                    256,
                                                                    5,
                                                                    rmp.m_texFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "fontTexture");

  rmp.m_renderTargetTexturesParams = yg::ResourceManager::TexturePoolParams(GetPlatform().TileSize(),
                                                                            GetPlatform().TileSize(),
                                                                            GetPlatform().MaxTilesCount(),
                                                                            rmp.m_rtFormat,
                                                                            true,
                                                                            true,
                                                                            false,
                                                                            4,
                                                                            "renderTargetTexture");

  rmp.m_styleCacheTexturesParams = yg::ResourceManager::TexturePoolParams(rmp.m_fontTexturesParams.m_texWidth * 2,
                                                                          rmp.m_fontTexturesParams.m_texHeight * 2,
                                                                          2,
                                                                          rmp.m_texFormat,
                                                                          true,
                                                                          true,
                                                                          true,
                                                                          1,
                                                                          "styleCacheTexture");

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
                                                                 GetPlatform().CpuCores() + 2,
                                                                 GetPlatform().CpuCores());

  rmp.m_useSingleThreadedOGL = true;
  rmp.m_useVA = !yg::gl::g_isBufferObjectsSupported;

  rmp.fitIntoLimits();

  m_maxTilesCount = rmp.m_renderTargetTexturesParams.m_texCount;

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
  p.m_useGuiResources = true;
  p.m_isSynchronized = false;

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setVideoTimer(videoTimer);
  m_windowHandle->setRenderContext(primaryRC);
}

void TilingRenderPolicyST::SetRenderFn(TRenderFn renderFn)
{
  RenderPolicy::SetRenderFn(renderFn);

  m_tileRenderer.reset(new TileRenderer(GetPlatform().SkinName(),
                                        m_maxTilesCount,
                                        1, //GetPlatform().CpuCores(),
                                        m_bgColor,
                                        renderFn,
                                        m_primaryRC,
                                        m_resourceManager,
                                        GetPlatform().VisualScale(),
                                        &m_glQueue));

  m_coverageGenerator.reset(new CoverageGenerator(GetPlatform().TileSize(),
                                                  GetPlatform().ScaleEtalonSize(),
                                                  m_tileRenderer.get(),
                                                  m_windowHandle,
                                                  m_primaryRC,
                                                  m_resourceManager,
                                                  &m_glQueue
                                                  ));
}

void TilingRenderPolicyST::BeginFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  m_IsDebugging = false;
  if (m_IsDebugging)
    LOG(LINFO, ("-------BeginFrame-------"));
}

void TilingRenderPolicyST::EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  ScreenCoverage * curCvg = &m_coverageGenerator->CurrentCoverage();
  curCvg->EndFrame(e->drawer()->screen().get());
  m_coverageGenerator->Mutex().Unlock();

  if (m_IsDebugging)
    LOG(LINFO, ("-------EndFrame-------"));
}

bool TilingRenderPolicyST::NeedRedraw() const
{
  return RenderPolicy::NeedRedraw() || !m_glQueue.Empty();
}

void TilingRenderPolicyST::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & currentScreen)
{
  m_resourceManager->mergeFreeResources();

  RenderQueuedCommands(e->drawer()->screen().get());

  m_resourceManager->mergeFreeResources();

  DrawerYG * pDrawer = e->drawer();

  pDrawer->screen()->clear(m_bgColor);

  m_coverageGenerator->AddCoverScreenTask(currentScreen);

  m_coverageGenerator->Mutex().Lock();

  ScreenCoverage * curCvg = &m_coverageGenerator->CurrentCoverage();

  curCvg->Draw(pDrawer->screen().get(), currentScreen);
}

TileRenderer & TilingRenderPolicyST::GetTileRenderer()
{
  return *m_tileRenderer.get();
}

void TilingRenderPolicyST::StartScale()
{
  m_isScaling = true;
}

void TilingRenderPolicyST::StopScale()
{
  m_isScaling = false;
}

bool TilingRenderPolicyST::IsTiling() const
{
  return true;
}

void TilingRenderPolicyST::RenderQueuedCommands(yg::gl::Screen * screen)
{
  if (!m_state)
  {
    m_state = screen->createState();
    m_state->m_isDebugging = m_IsDebugging;
  }

  screen->getState(m_state.get());

  m_curState = m_state;

  unsigned cmdProcessed = 0;
  unsigned const maxCmdPerFrame = 10000;

  m_glQueue.ProcessList(bind(&TilingRenderPolicyST::ProcessRenderQueue, this, _1, maxCmdPerFrame));

  cmdProcessed = m_frameGLQueue.size();

  for (list<yg::gl::Packet>::iterator it = m_frameGLQueue.begin(); it != m_frameGLQueue.end(); ++it)
  {
    if (it->m_state)
    {
      it->m_state->m_isDebugging = m_IsDebugging;
      it->m_state->apply(m_curState.get());
//      OGLCHECK(glFinish());
      m_curState = it->m_state;
    }
    if (it->m_command)
    {
      it->m_command->setIsDebugging(m_IsDebugging);
      it->m_command->perform();
//    OGLCHECK(glFinish());
    }

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
}

void TilingRenderPolicyST::ProcessRenderQueue(list<yg::gl::Packet> & renderQueue, int maxPackets)
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
      /// searching for "group boundary" markers

      list<yg::gl::Packet>::iterator first = renderQueue.begin();
      list<yg::gl::Packet>::iterator last = renderQueue.begin();

      int packetsLeft = maxPackets;

      while ((packetsLeft != 0) && (last != renderQueue.end()))
      {
        yg::gl::Packet p = *last;
        if (p.m_groupBoundary)
        {
          if (m_IsDebugging)
            LOG(LINFO, ("found frame boundary"));
          /// found frame boundary, copying
          copy(first, ++last, back_inserter(m_frameGLQueue));
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
