#pragma once

#include "../storage/storage_defines.hpp"

#include "../std/string.hpp"

class ActiveMapsLayout
{
public:
  enum class TGroup
  {
    ENewMap,
    EOutOfDate,
    EUpToDate
  };

  class ActiveMapsListener
  {
  public:
    /// if some country been inserted than oldGroup == newGroup, and oldPosition == -1
    /// if some country been deleted than oldGroup == newGroup, and newPosition == -1
    /// if group of country been changed. than oldGroup != newGroup, oldPosition >= 0 and newPosition >= 0
    virtual void CountryGroupChanged(TGroup const & oldGroup, int oldPosition,
                                     TGroup const & newGroup, int newPosition) = 0;
    virtual void CountryStatusChanged(TGroup const & group, int position) = 0;
    virtual void CountryOptionsChanged(TGroup const & group, int position) = 0;
    virtual void DownloadingProgressUpdate(TGroup const & group, int position,
                                           storage::LocalAndRemoteSizeT const & progress) = 0;
  };

  ActiveMapsLayout();

  int GetCountInGroup(TGroup const & group) const;
  string const & GetCountryName(TGroup const & group, int position) const;
  storage::TStatus GetCountryStatus(TGroup const & group, int position) const;
  storage::TMapOptions GetCountryOptions(TGroup const & group, int position) const;

  /// set to nullptr when go out from ActiveMaps activity
  void SetActiveMapsListener(ActiveMapsListener * listener);

  void ChangeCountryOptions(TGroup const & group, int position, storage::TMapOptions const & options);
  void ResumeDownloading(TGroup const & group, int position);

  void IsDownloadingActive() const;
  void CancelDownloading(TGroup const & group, int position);
  void CancelAllDownloading();

private:
  /// ENewMap    - [0, TRangeSplit.first)
  /// EOutOfDate - [TRangeSplit.first, TRangeSplit.second)
  /// EUpToDate  - [TRangeSplit.second, m_items.size)
  typedef pair<int, int> TRangeSplit;
};
