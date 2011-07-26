#include "rendering.h"

#include "../../yg/framebuffer.hpp"
#include "../../yg/internal/opengl.hpp"

#include "../../platform/platform.hpp"

#include "../../base/math.hpp"


shared_ptr<yg::ResourceManager> CreateResourceManager()
{
  int bigVBSize = pow(2, ceil(log(15000.0 * sizeof(yg::gl::Vertex)) / log(2)));
  int bigIBSize = pow(2, ceil(log(30000.0 * sizeof(unsigned short)) / log(2)));

  int smallVBSize = pow(2, ceil(log(1500.0 * sizeof(yg::gl::Vertex)) / log(2)));
  int smallIBSize = pow(2, ceil(log(3000.0 * sizeof(unsigned short)) / log(2)));

  int blitVBSize = pow(2, ceil(log(10.0 * sizeof(yg::gl::AuxVertex)) / log(2)));
  int blitIBSize = pow(2, ceil(log(10.0 * sizeof(unsigned short)) / log(2)));

  Platform & pl = GetPlatform();
  yg::ResourceManager * pRM = new yg::ResourceManager(
        bigVBSize, bigIBSize, 4,
        smallVBSize, smallIBSize, 10,
        blitVBSize, blitIBSize, 10,
        512, 256, 6,
        512, 256, 4,
        pl.TileSize(), pl.TileSize(), pl.MaxTilesCount(),
        "unicode_blocks.txt",
        "fonts_whitelist.txt",
        "fonts_blacklist.txt",
        1.5 * 1024 * 1024,
        GetPlatform().CpuCores() + 1,
        yg::Rt8Bpp,
        false);

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
  p.m_glyphCacheID = pRM->guiThreadGlyphCacheID();
  p.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(true));

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
