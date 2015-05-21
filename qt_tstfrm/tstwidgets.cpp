#include "qt_tstfrm/tstwidgets.hpp"

#include "render/render_policy.hpp"

#include "graphics/screen.hpp"
#include "graphics/resource_manager.hpp"

#include "graphics/opengl/utils.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "graphics/opengl/renderbuffer.hpp"
#include "graphics/opengl/opengl.hpp"

#include "platform/platform.hpp"

namespace tst
{
  void GLDrawWidget::initializeGL()
  {
    try
    {
      graphics::gl::InitExtensions();
      graphics::gl::CheckExtensionSupport();
    }
    catch (graphics::gl::platform_unsupported & e)
    {
      /// TODO: Show "Please Update Drivers" dialog and close the program.
    }

    m_primaryContext.reset(new qt::gl::RenderContext(this));

    graphics::ResourceManager::Params rmp;
    rmp.m_texFormat = graphics::Data8Bpp;

    rmp.m_videoMemoryLimit = 20 * 1024 * 1024;

    graphics::ResourceManager::StoragePoolParams spp;
    graphics::ResourceManager::TexturePoolParams tpp;

    spp = graphics::ResourceManager::StoragePoolParams(30000 * sizeof(graphics::gl::Vertex),
                                                       sizeof(graphics::gl::Vertex),
                                                       50000 * sizeof(unsigned short),
                                                       sizeof(unsigned short),
                                                       1,
                                                       graphics::ELargeStorage,
                                                       true);

    rmp.m_storageParams[graphics::ELargeStorage] = spp;

    spp = graphics::ResourceManager::StoragePoolParams(3000 * sizeof(graphics::gl::Vertex),
                                                       sizeof(graphics::gl::Vertex),
                                                       5000 * sizeof(unsigned short),
                                                       sizeof(unsigned short),
                                                       1,
                                                       graphics::EMediumStorage,
                                                       true);

    rmp.m_storageParams[graphics::EMediumStorage] = spp;

    spp = graphics::ResourceManager::StoragePoolParams(500 * sizeof(graphics::gl::Vertex),
                                                       sizeof(graphics::gl::Vertex),
                                                       500 * sizeof(unsigned short),
                                                       sizeof(unsigned short),
                                                       1,
                                                       graphics::ESmallStorage,
                                                       true);

    rmp.m_storageParams[graphics::ESmallStorage] = spp;

    spp = graphics::ResourceManager::StoragePoolParams(100 * sizeof(graphics::gl::Vertex),
                                                       sizeof(graphics::gl::Vertex),
                                                       200 * sizeof(unsigned short),
                                                       sizeof(unsigned short),
                                                       1,
                                                       graphics::ETinyStorage,
                                                       true);

    rmp.m_storageParams[graphics::ETinyStorage] = spp;


    tpp = graphics::ResourceManager::TexturePoolParams(512,
                                                       512,
                                                       1,
                                                       rmp.m_texFormat,
                                                       graphics::ELargeTexture,
                                                       true);

    rmp.m_textureParams[graphics::ELargeTexture] = tpp;

    tpp = graphics::ResourceManager::TexturePoolParams(256,
                                                       256,
                                                       1,
                                                       rmp.m_texFormat,
                                                       graphics::EMediumTexture,
                                                       true);

    rmp.m_textureParams[graphics::EMediumTexture] = tpp;

    tpp = graphics::ResourceManager::TexturePoolParams(128,
                                                       128,
                                                       1,
                                                       rmp.m_texFormat,
                                                       graphics::ESmallTexture,
                                                       true);

    rmp.m_textureParams[graphics::ESmallTexture] = tpp;


    rmp.m_glyphCacheParams = GetResourceGlyphCacheParams(graphics::EDensityMDPI);

    rmp.m_threadSlotsCount = 1;
    rmp.m_renderThreadsCount = 0;

    rmp.m_useSingleThreadedOGL = false;

    m_resourceManager.reset(new graphics::ResourceManager(rmp, "basic.skn", graphics::EDensityMDPI));

    m_primaryContext->setResourceManager(m_resourceManager);
    m_primaryContext->startThreadDrawing(0);

    Platform::FilesList fonts;
    GetPlatform().GetFontNames(fonts);
    m_resourceManager->addFonts(fonts);

    graphics::Screen::Params params;

    m_primaryFrameBuffer.reset(new graphics::gl::FrameBuffer(true));

    params.m_frameBuffer = m_primaryFrameBuffer;
    params.m_resourceManager = m_resourceManager;
    params.m_threadSlot = m_resourceManager->guiThreadSlot();
    params.m_renderContext = m_primaryContext;

    m_primaryScreen.reset(new graphics::Screen(params));
  }

  void GLDrawWidget::resizeGL(int w, int h)
  {
    m_primaryScreen->onSize(w, h);
    m_primaryFrameBuffer->onSize(w, h);
  }

  void GLDrawWidget::paintGL()
  {
    DoDraw(m_primaryScreen);
  }
}
