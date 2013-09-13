#include "yopme_render_policy.hpp"
#include "window_handle.hpp"

#include "../base/matrix.hpp"
#include "../indexer/scales.hpp"
#include "../geometry/screenbase.hpp"

#include "../graphics/opengl/framebuffer.hpp"
#include "../graphics/render_context.hpp"
#include "../graphics/blitter.hpp"
#include "../graphics/pen.hpp"
#include "../graphics/display_list.hpp"
#include "../platform/platform.hpp"

#include "../std/vector.hpp"

using namespace graphics;

YopmeRP::YopmeRP(RenderPolicy::Params const & p)
  : RenderPolicy(p, false, 1)
  , m_drawApiPin(false)
  , m_drawMyPosition(false)
{
  ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();

  double k = VisualScale();

  rmp.m_textureParams[ELargeTexture]        = GetTextureParam(512, 1, rmp.m_texFormat, ELargeTexture);
  rmp.m_textureParams[ESmallTexture]        = GetTextureParam(128 * k, 2, rmp.m_texFormat, ESmallTexture);

  rmp.m_storageParams[ELargeStorage]        = GetStorageParam(50000, 100000, 15, ELargeStorage);
  rmp.m_storageParams[EMediumStorage]       = GetStorageParam(6000, 9000, 1, EMediumStorage);
  rmp.m_storageParams[ESmallStorage]        = GetStorageParam(2000, 6000, 1, ESmallStorage);
  rmp.m_storageParams[ETinyStorage]         = GetStorageParam(100, 200, 1, ETinyStorage);

  rmp.m_glyphCacheParams = graphics::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                       "fonts_whitelist.txt",
                                                                       "fonts_blacklist.txt",
                                                                       2 * 1024 * 1024,
                                                                       Density());

  rmp.m_renderThreadsCount = 0;
  rmp.m_threadSlotsCount = 1;
  rmp.m_useSingleThreadedOGL = true;

  m_resourceManager.reset(new graphics::ResourceManager(rmp, SkinName(), Density()));

  m_primaryRC->setResourceManager(m_resourceManager);
  m_primaryRC->startThreadDrawing(m_resourceManager->guiThreadSlot());

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  m_drawer.reset(CreateDrawer(p.m_useDefaultFB, p.m_primaryRC, ESmallStorage, ESmallTexture));
  m_offscreenDrawer.reset(CreateDrawer(false, p.m_primaryRC, ELargeStorage, ELargeTexture));
  m_offscreenDrawer->screen()->setDepthBuffer(make_shared_ptr(new graphics::gl::RenderBuffer(p.m_screenWidth, p.m_screenHeight, true)));

  InitCacheScreen();
  InitWindowsHandle(p.m_videoTimer, p.m_primaryRC);
}

void YopmeRP::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  shared_ptr<graphics::gl::BaseTexture> renderTarget;

  int width = m_offscreenDrawer->screen()->width();
  int height = m_offscreenDrawer->screen()->height();

  ASSERT(width == GetDrawer()->screen()->width(), ());
  ASSERT(height == GetDrawer()->screen()->height(), ());

  shared_ptr<graphics::Overlay> overlay(new graphics::Overlay());
  overlay->setCouldOverlap(true);

  { // offscreen rendering
    m2::RectI renderRect(0, 0, width, height);

    graphics::Screen * pScreen = m_offscreenDrawer->screen();
    renderTarget = m_resourceManager->createRenderTarget(width, height);

    pScreen->setOverlay(overlay);
    pScreen->setRenderTarget(renderTarget);
    pScreen->beginFrame();
    pScreen->setClipRect(renderRect);
    pScreen->clear(m_bgColor);

    shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_offscreenDrawer.get()));

    size_t const scaleEtalonSize = 256;

    m2::RectD glbRect;
    m2::PointD const pxCenter = s.PixelRect().Center();

    s.PtoG(m2::RectD(pxCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                     pxCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
           glbRect);

    m_renderFn(paintEvent, s, m2::RectD(renderRect), scales::GetScaleLevel(glbRect));

    pScreen->endFrame();
    pScreen->resetOverlay();
    pScreen->unbindRenderTarget();
  }

  overlay->clip(m2::RectI(0, 0, width, height));
  shared_ptr<graphics::Overlay> drawOverlay(new graphics::Overlay());
  drawOverlay->setCouldOverlap(false);
  drawOverlay->merge(*overlay);

  {
    // on screen rendering
    graphics::Screen * pScreen = GetDrawer()->screen();
    graphics::BlitInfo info;
    info.m_srcSurface = renderTarget;
    info.m_srcRect = m2::RectI(0, 0, width, height);
    info.m_texRect = m2::RectU(1, 1, width - 1, height - 1);
    info.m_matrix = math::Identity<double, 3>();

    pScreen->beginFrame();
    pScreen->clear(m_bgColor);

    pScreen->applyBlitStates();
    pScreen->blit(&info, 1, true, graphics::minDepth);

    pScreen->applyStates();
    drawOverlay->draw(pScreen, math::Identity<double, 3>());

    if (m_drawMyPosition)
    {
      graphics::Circle::Info info(8, graphics::Color(0, 0, 0, 255), true, 3, graphics::Color(255, 255, 255, 255));
      pScreen->drawCircle(m_myPositionPoint, info, graphics::EPosCenter, graphics::maxDepth);
    }

    if (m_drawApiPin)
    {
      Pen::Info outlineInfo(graphics::Color(255, 255, 255, 255), 5);
      Pen::Info info(graphics::Color(0, 0, 0, 255), 3);

      uint32_t outlineID = pScreen->mapInfo(outlineInfo);
      uint32_t infoID = pScreen->mapInfo(info);

      m2::PointD line1[2] =
      {
        m_apiPinPoint - m2::PointD(5.0, 5.0),
        m_apiPinPoint + m2::PointD(5.0, 5.0)
      };

      m2::PointD line2[2] =
      {
        m_apiPinPoint - m2::PointD(5.0, -5.0),
        m_apiPinPoint + m2::PointD(5.0, -5.0)
      };

      pScreen->drawPath(line1, 2, 0.0, outlineID, graphics::maxDepth);
      pScreen->drawPath(line2, 2, 0.0, outlineID, graphics::maxDepth);
      pScreen->drawPath(line1, 2, 0.0, infoID, graphics::maxDepth);
      pScreen->drawPath(line2, 2, 0.0, infoID, graphics::maxDepth);
    }

    pScreen->endFrame();
  }
}

void YopmeRP::OnSize(int w, int h)
{
  RenderPolicy::OnSize(w, h);
  m_offscreenDrawer->onSize(w, h);
  m_offscreenDrawer->screen()->setDepthBuffer(make_shared_ptr(new graphics::gl::RenderBuffer(w, h, true)));
}

void YopmeRP::DrawApiPin(bool isNeed, m2::PointD const & point)
{
  m_drawApiPin = isNeed;
  m_apiPinPoint = point;
}

void YopmeRP::DrawMyLocation(bool isNeed, m2::PointD const & point)
{
  m_drawMyPosition = isNeed;
  m_myPositionPoint = point;
}
