#include "bookmark_balloon.hpp"
#include "balloon_manager.hpp"
#include "framework.hpp"

#include "../search/result.hpp"

#include "../graphics/depth_constants.hpp"

#include "../gui/controller.hpp"


BalloonManager::BalloonManager(Framework & f)
  : m_f(f), m_updateForLocation(false)
{
}

void BalloonManager::CreateBookmarkBalloon()
{
  CHECK(m_f.GetGuiController(), ());
  CHECK(m_f.GetLocationState(), ());

  BookmarkBalloon::Params bp;
  bp.m_position = graphics::EPosAbove;
  bp.m_depth = graphics::balloonBaseDepth;
  bp.m_pivot = m2::PointD(0, 0);
  bp.m_framework = &m_f;

  m_balloon.reset(new BookmarkBalloon(bp));
  m_balloon->setIsVisible(false);

  m_f.GetGuiController()->AddElement(m_balloon);
  m_f.GetLocationState()->AddOnPositionClickListener(bind(&BalloonManager::OnPositionClicked, this, _1));
}

void BalloonManager::RenderPolicyCreated(graphics::EDensity density)
{
  if (m_balloon == 0)
    CreateBookmarkBalloon();

  m_balloon->setImage(graphics::Image::Info("arrow.png", density));
}

void BalloonManager::ScreenSizeChanged(int width, int height)
{
  if (m_balloon)
    m_balloon->onScreenSize(width, height);
}

void BalloonManager::LocationChanged(location::GpsInfo const & info)
{
  if (m_balloon && m_updateForLocation)
  {
    m_balloon->setGlbPivot(m2::PointD(MercatorBounds::LonToX(info.m_longitude),
                                      MercatorBounds::LatToY(info.m_latitude)));
  }
}

void BalloonManager::OnPositionClicked(m2::PointD const & pt)
{
  Show(pt, m_f.GetStringsBundle().GetString("my_position"), "", false);

  m_balloon->setOnClickListener(bind(&BalloonManager::OnActivateMyPosition, this, _1));

  m_updateForLocation = true;
}

void BalloonManager::Show(m2::PointD const & pt, string const & name, string const & type, bool needPadding)
{
  m_updateForLocation = false;

  m_balloon->setGlbPivot(pt);
  m_balloon->setBookmarkCaption(name, type);
  m_balloon->showAnimated(needPadding);

  m_f.Invalidate();
}

void BalloonManager::Hide()
{
  m_updateForLocation = false;

  m_balloon->hide();

  m_f.Invalidate();
}

void BalloonManager::ShowAddress(m2::PointD const & pt, search::AddressInfo const & info)
{
  string name = info.GetPinName();
  string type = info.GetPinType();
  if (name.empty() && type.empty())
    name = m_f.GetStringsBundle().GetString("dropped_pin");

  Show(pt, name, type, false);

  m_balloon->setOnClickListener(bind(&BalloonManager::OnActivatePOI, this, _1, info));
}

void BalloonManager::ShowApiPoint(url_scheme::ApiPoint const & apiPoint)
{
  Show(m2::PointD(MercatorBounds::LonToX(apiPoint.m_lon),
                  MercatorBounds::LatToY(apiPoint.m_lat)),
       apiPoint.m_name, "", true);
  m_balloon->setOnClickListener(bind(&BalloonManager::OnActivateAPI, this, _1, apiPoint));
}

void BalloonManager::ShowBookmark(BookmarkAndCategory bmAndCat)
{
  Bookmark const * pBM = m_f.GetBmCategory(bmAndCat.first)->GetBookmark(bmAndCat.second);
  Show(pBM->GetOrg(), pBM->GetName(), "", true);
  m_balloon->setOnClickListener(bind(&BalloonManager::OnActivateBookmark, this, _1, bmAndCat));
}

void BalloonManager::OnClick(m2::PointD const & pxPoint, bool isLongTouch)
{
  url_scheme::ApiPoint apiPoint;
  if (m_f.GetMapApiPoint(pxPoint, apiPoint))
  {
    Show(m2::PointD(MercatorBounds::LonToX(apiPoint.m_lon),
                    MercatorBounds::LatToY(apiPoint.m_lat)),
         apiPoint.m_name, "", true);
    m_balloon->setOnClickListener(bind(&BalloonManager::OnActivateAPI, this, _1, apiPoint));
  }
  else
  {
    search::AddressInfo addrInfo;
    m2::PointD          pxPivot;
    BookmarkAndCategory bmAndCat;

    switch (m_f.GetBookmarkOrPoi(pxPoint, pxPivot, addrInfo, bmAndCat))
    {
    case Framework::BOOKMARK:
      {
        ShowBookmark(bmAndCat);
        return;
      }

    case Framework::POI:
      if (!m_balloon->isVisible())
      {
        ShowAddress(m_f.PtoG(pxPivot), addrInfo);
        return;
      }

    default:
      if (isLongTouch)
      {
        m2::PointD const glbPoint = m_f.PtoG(pxPoint);
        m_f.GetAddressInfoForGlobalPoint(glbPoint, addrInfo);
        ShowAddress(glbPoint, addrInfo);
        return;
      }
    }

    // hide the balloon by default if no any Show before
    Hide();
  }
}

void BalloonManager::ClearListeners()
{
  m_poiListener.clear();
  m_bookmarkListener.clear();
  m_apiListener.clear();
  m_positionListener.clear();
}

void BalloonManager::OnActivateMyPosition(gui::Element *)
{
  m2::PointD const & pt = m_balloon->glbPivot();
  m_positionListener(MercatorBounds::YToLat(pt.y),
                     MercatorBounds::XToLon(pt.x));
}

void BalloonManager::OnActivatePOI(gui::Element *, search::AddressInfo const & info)
{
  m_poiListener(m_balloon->glbPivot(), info);
}

void BalloonManager::OnActivateAPI(gui::Element *, url_scheme::ApiPoint const & apiPoint)
{
  m_apiListener(apiPoint);
}

void BalloonManager::OnActivateBookmark(gui::Element *, BookmarkAndCategory const & bmAndCat)
{
  m_bookmarkListener(bmAndCat);
}
