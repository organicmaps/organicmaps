#include "yopme_render_policy.hpp"
#include "window_handle.hpp"
#include "scales_processor.hpp"

#include "geometry/screenbase.hpp"

#include "graphics/opengl/framebuffer.hpp"
#include "graphics/render_context.hpp"
#include "graphics/blitter.hpp"
#include "graphics/pen.hpp"
#include "graphics/display_list.hpp"

#include "platform/platform.hpp"

#include "base/matrix.hpp"

#include "std/algorithm.hpp"
#include "std/vector.hpp"


using namespace graphics;

namespace
{
  const int DestinationDepthOffset = 10;
  const int ApiPinDepth = maxDepth - 10;
  const int MyLocationDepth = maxDepth;
  const int ApiPinLength = 5.0;

  class CrossElement : public OverlayElement
  {
  public:
    CrossElement(OverlayElement::Params const & params)
      : OverlayElement(params)
    {
      setIsFrozen(true);
    }

    virtual m2::RectD GetBoundRect() const
    {
      m2::PointD const offset(ApiPinLength, ApiPinLength);
      m2::PointD const & pt = pivot();
      return m2::RectD(pt - offset, pt + offset);
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
      r->drawPath(firstLine,  2, 0.0, outlineID, d - DestinationDepthOffset);
      r->drawPath(secondLine, 2, 0.0, outlineID, d - DestinationDepthOffset);
      r->drawPath(firstLine,  2, 0.0, infoID,    d);
      r->drawPath(secondLine, 2, 0.0, infoID,    d);
    }

    void setTransformation(math::Matrix<double, 3, 3> const & m)
    {
      OverlayElement::setTransformation(m);
    }
  };
}

YopmeRP::YopmeRP(RenderPolicy::Params const & p)
  : RenderPolicy(p, 1)
  , m_drawApiPin(false)
  , m_drawMyPosition(false)
{
  LOG(LDEBUG, ("Yopme render policy created"));
  fill(m_bgColors.begin(), m_bgColors.end(), graphics::Color(0xFF, 0xFF, 0xFF, 0xFF));

  ResourceManager::Params rmp = p.m_rmParams;

  rmp.checkDeviceCaps();

  double k = VisualScale();

  rmp.m_textureParams[ELargeTexture]        = GetTextureParam(512, 1, rmp.m_texFormat, ELargeTexture);
  rmp.m_textureParams[ESmallTexture]        = GetTextureParam(128 * k, 2, rmp.m_texFormat, ESmallTexture);

  rmp.m_storageParams[ELargeStorage]        = GetStorageParam(50000, 100000, 15, ELargeStorage);
  rmp.m_storageParams[EMediumStorage]       = GetStorageParam(6000, 9000, 1, EMediumStorage);
  rmp.m_storageParams[ESmallStorage]        = GetStorageParam(2000, 6000, 1, ESmallStorage);
  rmp.m_storageParams[ETinyStorage]         = GetStorageParam(100, 200, 1, ETinyStorage);

  rmp.m_glyphCacheParams = GetResourceGlyphCacheParams(Density());

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

  InitCacheScreen();
  InitWindowsHandle(p.m_videoTimer, p.m_primaryRC);
}

void YopmeRP::DrawCircle(Screen * pScreen, m2::PointD const & pt)
{
  {
    Circle::Info info(8.0, Color::White(), true, 2.0, Color::Black());
    pScreen->drawCircle(pt, info, EPosCenter, MyLocationDepth);
  }

  Pen::Info penInfo(Color::Black(), 2.0);
  uint32_t penID = pScreen->mapInfo(penInfo);

  Pen::Info boldPenInfo(Color::Black(), 5.0);
  uint32_t boldPenID = pScreen->mapInfo(boldPenInfo);

  Pen::Info outlinePenInfo(Color::White(), 3.0);
  uint32_t outlinePenID = pScreen->mapInfo(outlinePenInfo);

  const double offset = 12.0;

  {
    m2::PointD pts[2] =
    {
      m2::PointD(pt - m2::PointD(offset, 0.0)),
      m2::PointD(pt - m2::PointD(-offset, 0.0))
    };

    pScreen->drawPath(pts, 2, 0.0, boldPenID, MyLocationDepth - 7);
    pScreen->drawPath(pts, 2, 0.0, outlinePenID, MyLocationDepth - 5);
    pScreen->drawPath(pts, 2, 0.0, penID, MyLocationDepth);
  }

  {
    m2::PointD pts[2] =
    {
      m2::PointD(pt - m2::PointD(0.0, offset)),
      m2::PointD(pt - m2::PointD(0.0, -offset))
    };

    pScreen->drawPath(pts, 2, 0.0, boldPenID, MyLocationDepth - 7);
    pScreen->drawPath(pts, 2, 0.0, outlinePenID, MyLocationDepth - 5);
    pScreen->drawPath(pts, 2, 0.0, penID, MyLocationDepth);
  }

  {
    Circle::Info info(3.0, Color::White(), true, 3.0, Color::Black());
    pScreen->drawCircle(pt, info, EPosCenter, MyLocationDepth);
  }
}

void YopmeRP::InsertOverlayCross(m2::PointD pivot, shared_ptr<OverlayStorage> const & overlayStorage)
{
  OverlayElement::Params params;
  params.m_depth = ApiPinDepth;
  params.m_pivot = pivot;
  params.m_position = graphics::EPosCenter;
  overlayStorage->AddElement(make_shared<CrossElement>(params));
}

void YopmeRP::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
#ifndef USE_DRAPE
  shared_ptr<gl::BaseTexture> renderTarget;

  int width = m_offscreenDrawer->Screen()->width();
  int height = m_offscreenDrawer->Screen()->height();

  ASSERT(width == GetDrawer()->Screen()->width(), ());
  ASSERT(height == GetDrawer()->Screen()->height(), ());

  shared_ptr<OverlayStorage> overlay(new OverlayStorage(m2::RectD(0, 0, width, height)));

  { // offscreen rendering
    m2::RectI renderRect(0, 0, width, height);

    Screen * pScreen = m_offscreenDrawer->Screen();
    renderTarget = m_resourceManager->createRenderTarget(width, height);

    pScreen->setOverlay(overlay);
    pScreen->beginFrame();
    pScreen->setClipRect(renderRect);
    pScreen->clear(m_bgColors[0]);

    shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_offscreenDrawer.get()));
    m_renderFn(paintEvent, s, m2::RectD(renderRect), ScalesProcessor().GetTileScaleBase(s));

    pScreen->resetOverlay();

    if (m_drawApiPin)
      InsertOverlayCross(m_apiPinPoint, overlay);

    shared_ptr<Overlay> drawOverlay(new Overlay());
    drawOverlay->merge(overlay);

    pScreen->applySharpStates();
    pScreen->clear(m_bgColors[0], false);

    math::Matrix<double, 3, 3> const m = math::Identity<double, 3>();
    drawOverlay->forEach([&pScreen, &m](shared_ptr<OverlayElement> const & e)
    {
      e->draw(pScreen, m);
    });

    pScreen->endFrame();
    pScreen->copyFramebufferToImage(renderTarget);
  }

  {
    // on screen rendering
    Screen * pScreen = GetDrawer()->Screen();
    BlitInfo info;
    info.m_srcSurface = renderTarget;
    info.m_srcRect = m2::RectI(0, 0, width, height);
    info.m_texRect = m2::RectU(0, 0, width, height);
    info.m_matrix = math::Identity<double, 3>();
    info.m_depth = minDepth;

    pScreen->beginFrame();
    pScreen->clear(m_bgColors[0]);

    pScreen->applyBlitStates();
    pScreen->blit(&info, 1, true);

    pScreen->clear(m_bgColors[0], false);
    if (m_drawMyPosition)
      DrawCircle(pScreen, m_myPositionPoint);

    pScreen->endFrame();
  }
#endif // USE_DRAPE
}

void YopmeRP::OnSize(int w, int h)
{
  RenderPolicy::OnSize(w, h);
  m_offscreenDrawer->OnSize(w, h);
  m_offscreenDrawer->Screen()->setDepthBuffer(make_shared<gl::RenderBuffer>(w, h, true));
  m_offscreenDrawer->Screen()->setRenderTarget(make_shared<gl::RenderBuffer>(w, h, false));
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
