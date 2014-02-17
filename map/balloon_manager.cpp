#include "balloon_manager.hpp"
#include "framework.hpp"

#include "../search/result.hpp"

#include "../graphics/depth_constants.hpp"

#include "../gui/controller.hpp"


PinClickManager::PinClickManager(Framework & f)
  : m_f(f), m_updateForLocation(false)
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
  url_scheme::ResultPoint apiPoint;
  if (m_f.GetMapApiPoint(pxPoint, apiPoint))
  {
    OnActivateAPI(apiPoint);
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
        OnActivateBookmark(bmAndCat);
        return;
      }

    case Framework::ADDTIONAL_LAYER:
      {
        OnAdditonalLayer(bmAndCat.second);
        return;
      }

    case Framework::POI:
        OnActivatePOI(m_f.PtoG(pxPoint), addrInfo);
        return;

    default:
      if (isLongTouch)
      {
        m2::PointD const glbPoint = m_f.PtoG(pxPoint);
        m_f.GetAddressInfoForGlobalPoint(glbPoint, addrInfo);
        OnActivatePOI(glbPoint, addrInfo);
        return;
      }
    }

    OnDismiss();
  }
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
