#pragma once

#include "../storage/storage_defines.hpp"
#include "../storage/guides.hpp"
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
    ENewMap = 0,
    EOutOfDate = 1,
    EUpToDate = 2,
    EGroupCount = 3
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

  void UpdateAll();
  void CancelAll();

  int GetCountInGroup(TGroup const & group) const;
  string const & GetCountryName(TGroup const & group, int position) const;
  string const & GetCountryName(storage::TIndex const & index) const;
  TStatus GetCountryStatus(TGroup const & group, int position) const;
  TStatus GetCountryStatus(storage::TIndex const & index) const;
  TMapOptions GetCountryOptions(TGroup const & group, int position) const;
  TMapOptions GetCountryOptions(storage::TIndex const & index) const;
  LocalAndRemoteSizeT const GetDownloadableCountrySize(TGroup const & group, int position) const;
  LocalAndRemoteSizeT const GetDownloadableCountrySize(TIndex const & index) const;
  LocalAndRemoteSizeT const GetCountrySize(TGroup const & group, int position, TMapOptions const & options) const;
  LocalAndRemoteSizeT const GetCountrySize(TIndex const & index, TMapOptions const & options) const;

  int AddListener(ActiveMapsListener * listener);
  void RemoveListener(int slotID);

  bool GetGuideInfo(TGroup const & group, int position, guides::GuideInfo & info) const;

  void DownloadMap(TIndex const & index, TMapOptions const & options);
  void DownloadMap(TGroup const & group, int position, TMapOptions const & options);
  void DeleteMap(TIndex const & index, TMapOptions const & options);
  void DeleteMap(TGroup const & group, int position, TMapOptions const & options);
  void RetryDownloading(TGroup const & group, int position);
  void RetryDownloading(TIndex const & index);

  ///@{ For CountryStatusDisplay only
  TIndex const & GetCoreIndex(TGroup const & group, int position) const;
  string const GetFormatedCountryName(TIndex const & index);
  ///@}

  bool IsDownloadingActive() const;
  void CancelDownloading(TGroup const & group, int position);

  void ShowMap(TGroup const & group, int position);

private:
  friend class CountryTree;
  Storage const & GetStorage() const;
  Storage & GetStorage();

  bool GetGuideInfo(TIndex const & index, guides::GuideInfo & info) const;

  void ShowMap(TIndex const & index);
  void Init();

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
  Item const * FindItem(TIndex const & index) const;
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
  typedef pair<int, ActiveMapsListener *> TListenerNode;
  map<int, ActiveMapsListener *> m_listeners;
  int m_currentSlotID = 0;

  vector<Item> m_items;
  /// ENewMap    - [0, TRangeSplit.first)
  /// EOutOfDate - [TRangeSplit.first, TRangeSplit.second)
  /// EUpToDate  - [TRangeSplit.second, m_items.size)
  typedef pair<int, int> TRangeSplit;
  TRangeSplit m_split;
};

}
