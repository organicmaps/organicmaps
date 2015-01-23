#pragma once
#include "map/bookmark.hpp"

#include "geometry/point2d.hpp"

#include "std/function.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"


class Framework;
class PaintEvent;
namespace location { class GpsInfo; }
namespace gui { class Element; }

class PinClickManager
{
  Framework & m_f;

  void OnDismiss();

  void SetBalloonVisible(bool isVisible);

public:
  PinClickManager(Framework & f);

  void LocationChanged(location::GpsInfo const & info) {}

  void OnShowMark(UserMark const * mark);

  void Hide();

  void RemovePin();
  void Dismiss();

private:
  /// @name Platform dependent listeners to show special activities.
  //@{
  // You must delete UserMarkCopy obtained by this callback
  typedef function<void (unique_ptr<UserMarkCopy>)> TUserMarkListener;
  TUserMarkListener m_userMarkListener;
  typedef function<void (void)> TDismissListener;
  TDismissListener m_dismissListener;

public:
  template <class T> void ConnectUserMarkListener(T const & t)   { m_userMarkListener = t; }
  template <class T> void ConnectDismissListener(T const & t)    { m_dismissListener = t; }

  void ClearListeners();
  //@}
};
