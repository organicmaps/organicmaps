#pragma once
#include "bookmark.hpp"

#include "../graphics/defines.hpp"

#include "../geometry/point2d.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/function.hpp"


class Framework;
class BookmarkBalloon;
namespace location { class GpsInfo; }
namespace gui { class Element; }
namespace search { struct AddressInfo; }
namespace url_scheme
{
  struct ApiPoint;
  class ResultPoint;
}

class BalloonManager
{
  Framework & m_f;
  shared_ptr<BookmarkBalloon> m_balloon;
  bool m_updateForLocation;

  void CreateBookmarkBalloon();

  void OnPositionClicked(m2::PointD const & pt);
  void OnActivateMyPosition(gui::Element *);
  void OnActivatePOI(gui::Element *, search::AddressInfo const & info);
  void OnActivateAPI(gui::Element *, url_scheme::ResultPoint const & apiPoint);
  void OnActivateBookmark(gui::Element *, BookmarkAndCategory const & bmAndCat);

  void Show(m2::PointD const & pt, string const & name, string const & type, bool needPadding);

public:
  BalloonManager(Framework & f);

  void RenderPolicyCreated(graphics::EDensity density);
  void LocationChanged(location::GpsInfo const & info);
  void ScreenSizeChanged(int width, int height);

  void ShowAddress(m2::PointD const & pt, search::AddressInfo const & info);
  void ShowURLPoint(url_scheme::ResultPoint const & point, bool needPadding);
  void ShowBookmark(BookmarkAndCategory bmAndCat);

  void OnClick(m2::PointD const & pxPoint, bool isLongTouch);

  void Hide();

private:
  /// @name Platform dependent listeners to show special activities.
  //@{
  function<void (m2::PointD const &, search::AddressInfo const &)> m_poiListener;
  function<void (BookmarkAndCategory const &)> m_bookmarkListener;
  function<void (url_scheme::ApiPoint const &)> m_apiListener;
  function<void (double, double)> m_positionListener;

public:
  template <class T> void ConnectPoiListener(T const & t) { m_poiListener = t; }
  template <class T> void ConnectBookmarkListener(T const & t) { m_bookmarkListener = t; }
  template <class T> void ConnectApiListener(T const & t) { m_apiListener = t; }
  template <class T> void ConnectPositionListener(T const & t) { m_positionListener = t; }

  void ClearListeners();
  //@}
};
