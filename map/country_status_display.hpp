#pragma once

#include "../storage/storage.hpp"

#include "../gui/element.hpp"
#include "../gui/button.hpp"

#include "../std/unique_ptr.hpp"


namespace gui
{
  class TextView;
}

/// This class is a composite GUI element to display
/// an on-screen GUI for the country, which is not downloaded yet.
class CountryStatusDisplay : public gui::Element
{
private:

  /// Storage-related members and methods
  /// @{
  /// connection to the Storage for notifications
  unsigned m_slotID;
  storage::Storage * m_storage;
  /// notification callback upon country status change
  void CountryStatusChanged(storage::TIndex const &);
  /// notification callback upon country downloading progress
  void CountryProgress(storage::TIndex const &, pair<int64_t, int64_t> const & progress);
  /// @}

  void UpdateStatusAndProgress();

  /// download button
  unique_ptr<gui::Button> m_downloadButton;
  /// country status message
  unique_ptr<gui::TextView> m_statusMsg;
  /// current map name, "Province" part of the fullName
  string m_mapName;
  /// current map group name, "Country" part of the fullName
  string m_mapGroupName;
  /// current country status
  storage::TStatus m_countryStatus;
  /// index of the country in Storage
  storage::TIndex m_countryIdx;
  /// downloading progress of the country
  pair<int64_t, int64_t> m_countryProgress;

  bool m_notEnoughSpace;

  string const displayName() const;

  template <class T1, class T2>
  void SetStatusMessage(string const & msgID, T1 const * t1 = 0, T2 const * t2 = 0);

public:

  struct Params : public gui::Element::Params
  {
    storage::Storage * m_storage;
    Params();
  };

  CountryStatusDisplay(Params const & p);
  ~CountryStatusDisplay();

  /// start country download
  void downloadCountry();
  /// set download button listener
  void setDownloadListener(gui::Button::TOnClickListener const & l);
  /// set current country name
  void setCountryIndex(storage::TIndex const & idx);

  /// @name Override from graphics::OverlayElement and gui::Element.
  //@{
  virtual m2::RectD GetBoundRect() const;

  void setPivot(m2::PointD const & pv);
  void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

  void cache();
  void purge();
  void layout();

  void setController(gui::Controller * controller);

  bool onTapStarted(m2::PointD const & pt);
  bool onTapMoved(m2::PointD const & pt);
  bool onTapEnded(m2::PointD const & pt);
  bool onTapCancelled(m2::PointD const & pt);
  //@}
};
