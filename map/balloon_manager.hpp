#pragma once
#include "bookmark.hpp"

#include "../graphics/defines.hpp"

#include "../geometry/point2d.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/function.hpp"


class Framework;
class PaintEvent;
namespace location { class GpsInfo; }
namespace gui { class Element; }
namespace search { struct AddressInfo; }
namespace url_scheme
{
  struct ApiPoint;
  class ResultPoint;
}

class PinClickManager
{
  Framework & m_f;
  bool m_updateForLocation;

  void OnPositionClicked(m2::PointD const & pt);
  void OnActivateMyPosition();
  void OnActivatePOI(const m2::PointD & globalPoint, search::AddressInfo const & info);
  void OnActivateAPI(url_scheme::ResultPoint const & apiPoint);
  void OnActivateBookmark(BookmarkAndCategory const & bmAndCat);
  void OnAdditonalLayer(size_t index);
  void OnDismiss();

public:
  PinClickManager(Framework & f);

  void RenderPolicyCreated(graphics::EDensity density);
  void LocationChanged(location::GpsInfo const & info);
  void OnClick(m2::PointD const & pxPoint, bool isLongTouch);
  void Hide();

  void DrawPin(shared_ptr<PaintEvent> const & e);
  void RemovePin();
  void Dismiss();


private:
  bool m_hasPin;
  m2::PointD m_pinGlobalLocation;

private:
  /// @name Platform dependent listeners to show special activities.
  //@{
  function<void (m2::PointD const &, search::AddressInfo const &)> m_poiListener;
  function<void (BookmarkAndCategory const &)>                     m_bookmarkListener;
  function<void (url_scheme::ApiPoint const &)>                    m_apiListener;
  function<void (double, double)>                                  m_positionListener;
  function<void (size_t)>                                          m_additionalLayerListener;
  function<void (void)>                                            m_dismissListener;

public:
  template <class T> void ConnectPoiListener(T const & t)        { m_poiListener = t; }
  template <class T> void ConnectBookmarkListener(T const & t)   { m_bookmarkListener = t; }
  template <class T> void ConnectApiListener(T const & t)        { m_apiListener = t; }
  template <class T> void ConnectPositionListener(T const & t)   { m_positionListener = t; }
  template <class T> void ConnectAdditionalListener(T const & t) { m_additionalLayerListener = t; }
  template <class T> void ConnectDismissListener(T const & t)    { m_dismissListener = t; }

  void ClearListeners();
  //@}
};
