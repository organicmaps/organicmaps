#pragma once

#include "../storage/storage_defines.hpp"
#include "../storage/index.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"

class Framework;

namespace storage
{

class Storage;
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
                                           LocalAndRemoteSizeT const & progress) = 0;
  };

  ActiveMapsLayout(Framework & framework);
  ~ActiveMapsLayout();

  void Init();
  void UpdateAll();
  void CancelAll();

  int GetCountInGroup(TGroup const & group) const;
  string const & GetCountryName(TGroup const & group, int position) const;
  TStatus GetCountryStatus(TGroup const & group, int position) const;
  TMapOptions GetCountryOptions(TGroup const & group, int position) const;

  /// set to nullptr when go out from ActiveMaps activity
  void SetListener(ActiveMapsListener * listener);

  void DownloadMap(TIndex const & index, TMapOptions const & options);
  void DownloadMap(TGroup const & group, int position, TMapOptions const & options);
  void DeleteMap(TIndex const & index, TMapOptions const & options);
  void DeleteMap(TGroup const & group, int position, TMapOptions const & options);
  void RetryDownloading(TGroup const & group, int position);

  bool IsDownloadingActive() const;
  void CancelDownloading(TGroup const & group, int position);

private:
  friend class CountryTree;
  Storage const & GetStorage() const;
  Storage & GetStorage();

private:
  void StatusChangedCallback(TIndex const & index);
  void ProgressChangedCallback(TIndex const & index, LocalAndRemoteSizeT const & sizes);

private:
  struct Item
  {
    TIndex m_index;
    TStatus m_status;
    TMapOptions m_options;
    TMapOptions m_downloadRequest;
  };

  Item const & GetItemInGroup(TGroup const & group, int position) const;
  Item & GetItemInGroup(TGroup const & group, int position);
  int GetStartIndexInGroup(TGroup const & group) const;
  Item * FindItem(TIndex const & index);
  bool GetGroupAndPositionByIndex(TIndex const & index, TGroup & group, int & position);

private:
  int InsertInGroup(TGroup const & group, Item const & item);
  void DeleteFromGroup(TGroup const & group, int position);
  int MoveItemToGroup(TGroup const & group, int position, TGroup const & newGroup);

  void NotifyInsertion(TGroup const & group, int position);
  void NotifyDeletion(TGroup const & group, int position);
  void NotifyMove(TGroup const & oldGroup, int oldPosition,
                  TGroup const & newGroup, int newPosition);

  void NotifyStatusChanged(TGroup const & group, int position);
  void NotifyOptionsChanged(TGroup const & group, int position);

  TMapOptions ValidOptionsForDownload(TMapOptions const & options);
  TMapOptions ValidOptionsForDelete(TMapOptions const & options);

private:
  Framework & m_framework;
  int m_subscribeSlotID = 0;
  ActiveMapsListener * m_listener = nullptr;

  vector<Item> m_items;
  /// ENewMap    - [0, TRangeSplit.first)
  /// EOutOfDate - [TRangeSplit.first, TRangeSplit.second)
  /// EUpToDate  - [TRangeSplit.second, m_items.size)
  typedef pair<int, int> TRangeSplit;
  TRangeSplit m_split;
  bool m_inited = false;
};

}
