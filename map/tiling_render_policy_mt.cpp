#include "tiling_render_policy_mt.hpp"

#include "../platform/platform.hpp"

#include "../yg/internal/opengl.hpp"

#include "window_handle.hpp"
#include "tile_renderer.hpp"
#include "coverage_generator.hpp"

TilingRenderPolicyMT::TilingRenderPolicyMT(VideoTimer * videoTimer,
                                           bool useDefaultFB,
                                           yg::ResourceManager::Params const & rmParams,
                                           shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : BasicTilingRenderPolicy(primaryRC,
                            false,
                            false)
{
  yg::ResourceManager::Params rmp = rmParams;

  rmp.checkDeviceCaps();

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                       256,
                                                                       1,
                                                                       rmp.m_texFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture",
                                                                       false,
                                                                       true);

  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(50000 * sizeof(yg::gl::Vertex),
                                                                       sizeof(yg::gl::Vertex),
                                                                       100000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       5,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryStorage",
                                                                       false,
                                                                       true);

  rmp.m_smallStoragesParams = yg::ResourceManager::StoragePoolParams(6000 * sizeof(yg::gl::Vertex),
                                                                     sizeof(yg::gl::Vertex),
                                                                     9000 * sizeof(unsigned short),
                                                                     sizeof(unsigned short),
                                                                     1,
                                                                     true,
                                                                     true,
                                                                     1,
                                                                     "smallStorage",
                                                                     true,
                                                                     true);

  rmp.m_multiBlitStoragesParams = yg::ResourceManager::StoragePoolParams(500 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         500 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         10,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "multiBlitStorage",
                                                                         false,
                                                                         false);

  rmp.m_renderTargetTexturesParams = yg::ResourceManager::TexturePoolParams(GetPlatform().TileSize(),
                                                                            GetPlatform().TileSize(),
                                                                            1,
                                                                            rmp.m_texRtFormat,
                                                                            true,
                                                                            true,
                                                                            true,
                                                                            5,
                                                                            "renderTargetTexture",
                                                                            false,
                                                                            true);

  rmp.m_styleCacheTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                          512 * int(ceil(GetPlatform().VisualScale())),
                                                                          2,
                                                                          rmp.m_texFormat,
                                                                          true,
                                                                          true,
                                                                          true,
                                                                          1,
                                                                          "styleCacheTexture",
                                                                          false,
                                                                          false);

  rmp.m_guiThreadStoragesParams = yg::ResourceManager::StoragePoolParams(2000 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         4000 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         20,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "guiThreadStorage",
                                                                         true,
                                                                         false);

  rmp.m_guiThreadTexturesParams = yg::ResourceManager::TexturePoolParams(256,
                                                                         128,
                                                                         4,
                                                                         rmp.m_texFormat,
                                                                         true,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "guiThreadTexture",
                                                                         false,
                                                                         false);

  rmp.m_glyphCacheParams = yg::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 GetPlatform().CpuCores() + 2,
                                                                 GetPlatform().CpuCores(),
                                                                 0);

  rmp.m_useSingleThreadedOGL = false;
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
  p.m_useGuiResources = true;
  p.m_isSynchronized = false;

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setRenderPolicy(this);
  m_windowHandle->setVideoTimer(videoTimer);
  m_windowHandle->setRenderContext(primaryRC);
}

TilingRenderPolicyMT::~TilingRenderPolicyMT()
{
  LOG(LINFO, ("cancelling ResourceManager"));
  m_resourceManager->cancel();

  m_CoverageGenerator.reset();

  m_TileRenderer->ClearCommands();
  m_TileRenderer->SetSequenceID(numeric_limits<int>::max());
  m_TileRenderer->CancelCommands();
  m_TileRenderer->WaitForEmptyAndFinished();

  m_TileRenderer.reset();
}

void TilingRenderPolicyMT::SetRenderFn(TRenderFn renderFn)
{
  string skinName = GetPlatform().SkinName();

  m_TileRenderer.reset(new TileRenderer(skinName,
                                        GetPlatform().CpuCores(),
                                        m_bgColor,
                                        renderFn,
                                        m_primaryRC,
                                        m_resourceManager,
                                        GetPlatform().VisualScale(),
                                        0));

  m_CoverageGenerator.reset(new CoverageGenerator(skinName,
                                                  m_TileRenderer.get(),
                                                  m_windowHandle,
                                                  m_primaryRC,
                                                  m_resourceManager,
                                                  0,
                                                  m_emptyModelFn));
}
