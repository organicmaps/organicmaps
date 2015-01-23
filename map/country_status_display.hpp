#pragma once

#include "map/active_maps_layout.hpp"

#include "storage/storage_defines.hpp"

#include "std/unique_ptr.hpp"
#include "std/target_os.hpp"

class Framework;

namespace storage { struct TIndex; }

/// This class is a composite GUI element to display
/// an on-screen GUI for the country, which is not downloaded yet.
class CountryStatusDisplay : public storage::ActiveMapsLayout::ActiveMapsListener
{
public:
  struct Params
  {
    Params(storage::ActiveMapsLayout & activeMaps) : m_activeMaps(activeMaps) {}

    storage::ActiveMapsLayout & m_activeMaps;
  };

  CountryStatusDisplay(Params const & p);
  ~CountryStatusDisplay();

  /// set current country name
  void SetCountryIndex(storage::TIndex const & idx);
  typedef function<void (storage::TIndex const & idx, int)> TDownloadCountryFn;
  void SetDownloadCountryListener(TDownloadCountryFn const & fn) { m_downloadCallback = fn; }

  /// @name Override from graphics::OverlayElement and gui::Element.
  //@{
  virtual void setIsVisible(bool isVisible) const;
  virtual void setIsDirtyLayout(bool isDirty) const;

  void cache();
  void purge();
  void layout();
  //@}

private:
  virtual void CountryGroupChanged(storage::ActiveMapsLayout::TGroup const & oldGroup, int oldPosition,
                                   storage::ActiveMapsLayout::TGroup const & newGroup, int newPosition) {}
  virtual void CountryStatusChanged(storage::ActiveMapsLayout::TGroup const & group, int position,
                                    storage::TStatus const & oldStatus, storage::TStatus const & newStatus);
  virtual void CountryOptionsChanged(storage::ActiveMapsLayout::TGroup const & group, int position,
                                     MapOptions const & oldOpt, MapOptions const & newOpt)
  {
  }
  virtual void DownloadingProgressUpdate(storage::ActiveMapsLayout::TGroup const & group, int position,
                                         storage::LocalAndRemoteSizeT const & progress);

  template <class T1, class T2>
  string FormatStatusMessage(string const & msgID, T1 const * t1 = 0, T2 const * t2 = 0);

  void FormatDisplayName(string const & mapName, string const & groupName);

  void SetVisibilityForState() const;
  void SetContentForState();
  void SetContentForDownloadPropose();
  void SetContentForProgress();
  void SetContentForInQueue();
  void SetContentForError();

  void ComposeElementsForState();

  ///@TODO UVR
  //typedef function<bool (unique_ptr<gui::Button> const &, m2::PointD const &)> TTapActionFn;
  //bool OnTapAction(TTapActionFn const & action, m2::PointD const & pt);
  //void OnButtonClicked(Element const * button);

  bool IsStatusFailed() const;

private:
  storage::ActiveMapsLayout & m_activeMaps;
  int m_activeMapsSlotID = 0;

  string m_displayMapName;
  mutable storage::TStatus m_countryStatus = storage::TStatus::EUnknown;
  storage::TIndex m_countryIdx;
  storage::LocalAndRemoteSizeT m_progressSize;

  TDownloadCountryFn m_downloadCallback;
};
