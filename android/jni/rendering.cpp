#include "rendering.h"

#include "../../yg/framebuffer.hpp"
#include "../../yg/internal/opengl.hpp"

#include "../../platform/platform.hpp"

#include "../../base/math.hpp"


shared_ptr<yg::ResourceManager> CreateResourceManager()
{
  int bigVBSize = pow(2, ceil(log(15000 * sizeof(yg::gl::Vertex))));
  int bigIBSize = pow(2, ceil(log(30000 * sizeof(unsigned short))));

  int smallVBSize = pow(2, ceil(log(1500 * sizeof(yg::gl::Vertex))));
  int smallIBSize = pow(2, ceil(log(3000 * sizeof(unsigned short))));

  int blitVBSize = pow(2, ceil(log(10 * sizeof(yg::gl::AuxVertex))));
  int blitIBSize = pow(2, ceil(log(10 * sizeof(unsigned short))));

  Platform & pl = GetPlatform();
  yg::ResourceManager * pRM = new yg::ResourceManager(
        bigVBSize, bigIBSize, 4,
        smallVBSize, smallIBSize, 10,
        blitVBSize, blitIBSize, 10,
        512, 256, 6,
        512, 256, 4,
        "unicode_blocks.txt",
        "fonts_whitelist.txt",
        "fonts_blacklist.txt",
        1 * 1024 * 1024,
        500 * 1024,
        yg::Rt8Bpp,
        !yg::gl::g_isBufferObjectsSupported,
        !pl.IsMultiSampled());

  Platform::FilesList fonts;
  pl.GetFontNames(fonts);
  pRM->addFonts(fonts);

  return make_shared_ptr(pRM);
}

shared_ptr<DrawerYG> CreateDrawer(shared_ptr<yg::ResourceManager> pRM)
{
  Platform & pl = GetPlatform();

  DrawerYG::params_t p;
  p.m_resourceManager = pRM;
  p.m_isMultiSampled = pl.IsMultiSampled();
  p.m_frameBuffer.reset(new yg::gl::FrameBuffer());
  p.m_glyphCacheID = 1;

  return make_shared_ptr(new DrawerYG(pl.SkinName(), p));
}

namespace
{
  class AndroidRenderContext : public yg::gl::RenderContext
  {
  public:
    AndroidRenderContext()
    {
    }

    virtual void makeCurrent()
    {
    }

    virtual shared_ptr<yg::gl::RenderContext> createShared()
    {
      return make_shared_ptr(new AndroidRenderContext());
    }

    virtual void endThreadDrawing()
    {
    }
  };
}

shared_ptr<yg::gl::RenderContext> CreateRenderContext()
{
  return make_shared_ptr(new AndroidRenderContext());
}
