#include "yopme_render_policy.hpp"
#include "window_handle.hpp"
#include "scales_processor.hpp"

#include "../geometry/screenbase.hpp"

#include "../graphics/opengl/framebuffer.hpp"
#include "../graphics/render_context.hpp"
#include "../graphics/blitter.hpp"
#include "../graphics/pen.hpp"
#include "../graphics/display_list.hpp"

#include "../platform/platform.hpp"

#include "../base/matrix.hpp"

#include "../std/vector.hpp"


using namespace graphics;

namespace
{
  const int ApiPinDepth = maxDepth - 10;
  const int MyLocationDepth = maxDepth;
  const int ApiPinLength = 5.0;

  class CrossElement : public OverlayElement
  {
  public:
    CrossElement(OverlayElement::Params const & params)
      : OverlayElement(params)
    {
      m2::PointD offset(ApiPinLength, ApiPinLength);
      m2::PointD const & pt = pivot();
      m2::RectD r(pt - offset, pt + offset);

      m_boundRects.push_back(m2::AnyRectD(r));
      setIsFrozen(true);
    }

    vector<m2::AnyRectD> const & boundRects() const
    {
      return m_boundRects;
    }

    void draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const
    {
      Pen::Info outlineInfo(Color::White(), 5);
      Pen::Info info(Color::Black(), 3);

      uint32_t outlineID = r->mapInfo(outlineInfo);
      uint32_t infoID = r->mapInfo(info);

      m2::PointD const & pt = pivot();

      m2::PointD firstLineOffset(ApiPinLength, ApiPinLength);
      m2::PointD firstLine[2] =
      {
        pt - firstLineOffset,
        pt + firstLineOffset
      };

      m2::PointD secondLineOffset(ApiPinLength, -ApiPinLength);
      m2::PointD secondLine[2] =
      {
        pt - secondLineOffset,
        pt + secondLineOffset
      };

      double d = depth();
      r->drawPath(firstLine,  2, 0.0, outlineID, d);
      r->drawPath(secondLine, 2, 0.0, outlineID, d);
      r->drawPath(firstLine,  2, 0.0, infoID,    d);
      r->drawPath(secondLine, 2, 0.0, infoID,    d);
    }

    void setTransformation(math::Matrix<double, 3, 3> const & m)
    {
      OverlayElement::setTransformation(m);
    }

  private:
    vector<m2::AnyRectD> m_boundRects;
  };
}

YopmeRP::YopmeRP(RenderPolicy::Params const & p)
  : RenderPolicy(p, false, 1)
  , m_drawApiPin(false)
  , m_drawMyPosition(false)
{
  LOG(LDEBUG, ("Yopme render policy created"));
  m_bgColor = graphics::Color(0x77, 0x77, 0x77, 0xFF);

  ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();

  double k = VisualScale();

  rmp.m_textureParams[ELargeTexture]        = GetTextureParam(512, 1, rmp.m_texFormat, ELargeTexture);
  rmp.m_textureParams[ESmallTexture]        = GetTextureParam(128 * k, 2, rmp.m_texFormat, ESmallTexture);

  rmp.m_storageParams[ELargeStorage]        = GetStorageParam(50000, 100000, 15, ELargeStorage);
  rmp.m_storageParams[EMediumStorage]       = GetStorageParam(6000, 9000, 1, EMediumStorage);
  rmp.m_storageParams[ESmallStorage]        = GetStorageParam(2000, 6000, 1, ESmallStorage);
  rmp.m_storageParams[ETinyStorage]         = GetStorageParam(100, 200, 1, ETinyStorage);

  rmp.m_glyphCacheParams = ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                             "fonts_whitelist.txt",
                                                             "fonts_blacklist.txt",
                                                             2 * 1024 * 1024,
                                                             Density());

  rmp.m_renderThreadsCount = 0;
  rmp.m_threadSlotsCount = 1;
  rmp.m_useSingleThreadedOGL = true;

  m_resourceManager.reset(new ResourceManager(rmp, SkinName(), Density()));

  m_primaryRC->setResourceManager(m_resourceManager);
  m_primaryRC->startThreadDrawing(m_resourceManager->guiThreadSlot());

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  m_drawer.reset(CreateDrawer(p.m_useDefaultFB, p.m_primaryRC, ESmallStorage, ESmallTexture));
  m_offscreenDrawer.reset(CreateDrawer(false, p.m_primaryRC, ELargeStorage, ELargeTexture));
  m_offscreenDrawer->screen()->setDepthBuffer(make_shared_ptr(new gl::RenderBuffer(p.m_screenWidth, p.m_screenHeight, true)));

  InitCacheScreen();
  InitWindowsHandle(p.m_videoTimer, p.m_primaryRC);
}

void YopmeRP::DrawCircle(Screen * pScreen, m2::PointD const & pt)
{
  {
    Circle::Info info(8, Color::Black(), true, 2, Color::White());
    pScreen->drawCircle(pt, info, EPosCenter, MyLocationDepth - 5);
  }

  {
    Circle::Info info(4, Color::Black(), true, 2, Color::White());
    pScreen->drawCircle(pt, info, EPosCenter, MyLocationDepth);
  }

  {
    Circle::Info info(2, Color::White(), false);
    pScreen->drawCircle(pt, info, EPosCenter, MyLocationDepth);
  }
}

void YopmeRP::InsertOverlayCross(m2::PointD pivot, Overlay * overlay)
{
  OverlayElement::Params params;
  params.m_depth = ApiPinDepth;
  params.m_pivot = pivot;
  params.m_position = graphics::EPosCenter;
  overlay->processOverlayElement(make_shared_ptr(new CrossElement(params)));
}

void YopmeRP::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  shared_ptr<gl::BaseTexture> renderTarget;

  int width = m_offscreenDrawer->screen()->width();
  int height = m_offscreenDrawer->screen()->height();

  ASSERT(width == GetDrawer()->screen()->width(), ());
  ASSERT(height == GetDrawer()->screen()->height(), ());

  shared_ptr<Overlay> overlay(new Overlay());
  overlay->setCouldOverlap(true);

  { // offscreen rendering
    m2::RectI renderRect(0, 0, width, height);

    Screen * pScreen = m_offscreenDrawer->screen();
    renderTarget = m_resourceManager->createRenderTarget(width, height);

    pScreen->setOverlay(overlay);
    pScreen->setRenderTarget(renderTarget);
    pScreen->beginFrame();
    pScreen->setClipRect(renderRect);
    pScreen->clear(m_bgColor);

    shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_offscreenDrawer.get()));

    m_renderFn(paintEvent, s, m2::RectD(renderRect), ScalesProcessor().GetTileScaleBase(s));

    pScreen->endFrame();
    pScreen->resetOverlay();
    pScreen->unbindRenderTarget();
  }

  overlay->clip(m2::RectI(0, 0, width, height));
  shared_ptr<Overlay> drawOverlay(new Overlay());
  drawOverlay->setCouldOverlap(false);

  if (m_drawApiPin)
    InsertOverlayCross(m_apiPinPoint, drawOverlay.get());

  drawOverlay->merge(*overlay);

  {
    // on screen rendering
    Screen * pScreen = GetDrawer()->screen();
    BlitInfo info;
    info.m_srcSurface = renderTarget;
    info.m_srcRect = m2::RectI(0, 0, width, height);
    info.m_texRect = m2::RectU(1, 1, width - 1, height - 1);
    info.m_matrix = math::Identity<double, 3>();

    pScreen->beginFrame();
    pScreen->clear(m_bgColor);

    pScreen->applyBlitStates();
    pScreen->blit(&info, 1, true, minDepth);

    pScreen->applyStates();
    drawOverlay->draw(pScreen, math::Identity<double, 3>());

    if (m_drawMyPosition)
      DrawCircle(pScreen, m_myPositionPoint);

    pScreen->endFrame();
  }
}

void YopmeRP::OnSize(int w, int h)
{
  RenderPolicy::OnSize(w, h);
  m_offscreenDrawer->onSize(w, h);
  m_offscreenDrawer->screen()->setDepthBuffer(make_shared_ptr(new gl::RenderBuffer(w, h, true)));
}

void YopmeRP::SetDrawingApiPin(bool isNeed, m2::PointD const & point)
{
  m_drawApiPin = isNeed;
  m_apiPinPoint = point;
}

void YopmeRP::SetDrawingMyLocation(bool isNeed, m2::PointD const & point)
{
  m_drawMyPosition = isNeed;
  m_myPositionPoint = point;
}
