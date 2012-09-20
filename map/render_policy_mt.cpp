#include "render_policy_mt.hpp"

#include "../yg/internal/opengl.hpp"
#include "../yg/skin.hpp"

#include "../platform/platform.hpp"

#include "window_handle.hpp"
#include "render_queue.hpp"

RenderPolicyMT::RenderPolicyMT(RenderPolicy::Params const & p)
  : BasicRenderPolicy(p, false, 1)
{
  yg::ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();

  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(5000 * sizeof(yg::gl::Vertex),
                                                                       sizeof(yg::gl::Vertex),
                                                                       10000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       7,
                                                                       false,
                                                                       true,
                                                                       10,
                                                                       "primaryStorage",
                                                                       false,
                                                                       false);

  rmp.m_smallStoragesParams = yg::ResourceManager::StoragePoolParams(500 * sizeof(yg::gl::Vertex),
                                                                     sizeof(yg::gl::Vertex),
                                                                     1000 * sizeof(unsigned short),
                                                                     sizeof(unsigned short),
                                                                     7,
                                                                     false,
                                                                     true,
                                                                     5,
                                                                     "smallStorage",
                                                                     false,
                                                                     false);

  rmp.m_blitStoragesParams = yg::ResourceManager::StoragePoolParams(10 * sizeof(yg::gl::Vertex),
                                                                    sizeof(yg::gl::Vertex),
                                                                    10 * sizeof(unsigned short),
                                                                    sizeof(unsigned short),
                                                                    7,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "blitStorage",
                                                                    false,
                                                                    false);

  rmp.m_guiThreadStoragesParams = yg::ResourceManager::StoragePoolParams(300 * sizeof(yg::gl::Vertex),
                                                                    sizeof(yg::gl::Vertex),
                                                                    600 * sizeof(unsigned short),
                                                                    sizeof(unsigned short),
                                                                    7,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "guiThreadStorage",
                                                                    false,
                                                                    false);

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                       256,
                                                                       7,
                                                                       rmp.m_texFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture",
                                                                       false,
                                                                       false);

  rmp.m_fontTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                    256,
                                                                    7,
                                                                    rmp.m_texFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "fontTexture",
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
                                                                 3,
                                                                 1);

  rmp.m_useSingleThreadedOGL = false;

  rmp.fitIntoLimits();

  m_resourceManager.reset(new yg::ResourceManager(rmp));

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  DrawerYG::Params dp;

  dp.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(p.m_useDefaultFB));
  dp.m_resourceManager = m_resourceManager;
  dp.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  dp.m_skin = make_shared_ptr(yg::loadSkin(m_resourceManager, SkinName()));
  dp.m_visualScale = VisualScale();
  dp.m_isSynchronized = false;
  dp.m_useGuiResources = true;

  m_drawer.reset(new DrawerYG(dp));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setRenderPolicy(this);
  m_windowHandle->setVideoTimer(p.m_videoTimer);
  m_windowHandle->setRenderContext(p.m_primaryRC);

  m_RenderQueue.reset(new RenderQueue(SkinName(),
                false,
                true,
                0.1,
                false,
                ScaleEtalonSize(),
                VisualScale(),
                m_bgColor));

  m_RenderQueue->AddWindowHandle(m_windowHandle);
}
