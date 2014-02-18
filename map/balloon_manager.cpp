#include "balloon_manager.hpp"
#include "framework.hpp"

#include "../search/result.hpp"

#include "../graphics/depth_constants.hpp"

#include "../gui/controller.hpp"


PinClickManager::PinClickManager(Framework & f)
  : m_f(f)
  , m_updateForLocation(false)
  , m_hasPin(false)
{}

void PinClickManager::RenderPolicyCreated(graphics::EDensity density)
{}

void PinClickManager::LocationChanged(location::GpsInfo const & info)
{}

void PinClickManager::OnPositionClicked(m2::PointD const & pt)
{
  m_positionListener(pt.x, pt.y);
  m_updateForLocation = true;
}

void PinClickManager::Hide()
{
  m_updateForLocation = false;
  m_f.Invalidate();
}

void PinClickManager::OnClick(m2::PointD const & pxPoint, bool isLongTouch)
{
  // API
  url_scheme::ResultPoint apiPoint;
  if (m_f.GetMapApiPoint(pxPoint, apiPoint))
  {
    // @todo draw pin here
    OnActivateAPI(apiPoint);
    return;
  }

  // Everything else
  search::AddressInfo addrInfo;
  m2::PointD          pxPivot;
  BookmarkAndCategory bmAndCat;

  // By default we assume that we have pin
  m_hasPin = true;
  bool dispatched = false;

  switch (m_f.GetBookmarkOrPoi(pxPoint, pxPivot, addrInfo, bmAndCat))
  {
    case Framework::BOOKMARK:
    {
      OnActivateBookmark(bmAndCat);
      m_pinGlobalLocation = m_f.GetBmCategory(bmAndCat.first)->GetBookmark(bmAndCat.second)->GetOrg();
      dispatched = true;
      break;
    }

    case Framework::ADDTIONAL_LAYER:
    {
      OnAdditonalLayer(bmAndCat.second);
      m_pinGlobalLocation = m_f.GetBookmarkManager().AdditionalPoiLayerGetBookmark(bmAndCat.second)->GetOrg();
      dispatched = true;
      break;
    }

    case Framework::POI:
    {
      m2::PointD globalPoint = m_f.PtoG(pxPivot);
      OnActivatePOI(globalPoint, addrInfo);
      m_pinGlobalLocation = globalPoint;
      dispatched = true;
      break;
    }

    default:
    {
      if (isLongTouch)
      {
        m2::PointD const glbPoint = m_f.PtoG(pxPoint);
        m_f.GetAddressInfoForGlobalPoint(glbPoint, addrInfo);
        OnActivatePOI(glbPoint, addrInfo);
        m_pinGlobalLocation = glbPoint;
        dispatched = true;
      }
    }
  }

  if (!dispatched)
  {
    OnDismiss();
    m_hasPin = false;
  }

  m_f.Invalidate();
}

void PinClickManager::DrawPin(const shared_ptr<PaintEvent> & e)
{
  if (m_hasPin)
  {
    Navigator const    & navigator = m_f.GetNavigator();
    InformationDisplay & informationDisplay = m_f.GetInformationDisplay();
    m2::AnyRectD const & glbRect = navigator.Screen().GlobalRect();

    // @todo change pin picture
    if (glbRect.IsPointInside(m_pinGlobalLocation))
      informationDisplay.drawPlacemark(e->drawer(), "api_pin", navigator.GtoP(m_pinGlobalLocation));
  }
}

void PinClickManager::RemovePin()
{
  m_hasPin = false;
  m_f.Invalidate();
}

void PinClickManager::Dismiss()
{
  OnDismiss();
}

void PinClickManager::ClearListeners()
{
  m_poiListener.clear();
  m_bookmarkListener.clear();
  m_apiListener.clear();
  m_positionListener.clear();
  m_additionalLayerListener.clear();
  m_dismissListener.clear();
}

void PinClickManager::OnActivateMyPosition()
{
  m_positionListener(0,0);
}

void PinClickManager::OnActivatePOI(m2::PointD const & globalPoint, search::AddressInfo const & info)
{
  m_poiListener(globalPoint, info);
}

void PinClickManager::OnActivateAPI(url_scheme::ResultPoint const & apiPoint)
{
  m_apiListener(apiPoint.GetPoint());
}

void PinClickManager::OnActivateBookmark(BookmarkAndCategory const & bmAndCat)
{
  m_bookmarkListener(bmAndCat);
}

void PinClickManager::OnAdditonalLayer(size_t index)
{
  m_additionalLayerListener(index);
}

void PinClickManager::OnDismiss()
{
  m_dismissListener();
}
