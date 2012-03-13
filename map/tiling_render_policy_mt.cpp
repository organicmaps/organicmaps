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
                            GetPlatform().CpuCores())
{
  yg::ResourceManager::Params rmp = rmParams;

  rmp.selectTexRTFormat();

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                       256,
                                                                       10,
                                                                       rmp.m_texFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture",
                                                                       false,
                                                                       false);

  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(50000 * sizeof(yg::gl::Vertex),
                                                                       sizeof(yg::gl::Vertex),
                                                                       100000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       15,
                                                                       false,
                                                                       true,
                                                                       1,
                                                                       "primaryStorage",
                                                                       false,
                                                                       false);

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
                                                                            GetPlatform().MaxTilesCount(),
                                                                            rmp.m_texRtFormat,
                                                                            true,
                                                                            true,
                                                                            false,
                                                                            5,
                                                                            "renderTargetTexture",
                                                                            false,
                                                                            false);

  rmp.m_styleCacheTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                          1024,
                                                                          2,
                                                                          rmp.m_texFormat,
                                                                          true,
                                                                          true,
                                                                          true,
                                                                          1,
                                                                          "styleCacheTexture",
                                                                          false,
                                                                          false);

  rmp.m_guiThreadStoragesParams = yg::ResourceManager::StoragePoolParams(5000 * sizeof(yg::gl::Vertex),
                                                                         sizeof(yg::gl::Vertex),
                                                                         10000 * sizeof(unsigned short),
                                                                         sizeof(unsigned short),
                                                                         10,
                                                                         true,
                                                                         true,
                                                                         1,
                                                                         "guiThreadStorage",
                                                                         false,
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
  rmp.m_useVA = !yg::gl::g_isBufferObjectsSupported;

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
  m_TileRenderer.reset(new TileRenderer(GetPlatform().SkinName(),
                                        GetPlatform().MaxTilesCount(),
                                        GetPlatform().CpuCores(),
                                        m_bgColor,
                                        renderFn,
                                        m_primaryRC,
                                        m_resourceManager,
                                        GetPlatform().VisualScale(),
                                        0));

  m_CoverageGenerator.reset(new CoverageGenerator(m_TileRenderer.get(),
                                                  m_windowHandle,
                                                  m_primaryRC,
                                                  m_resourceManager,
                                                  0));
}
