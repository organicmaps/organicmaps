#pragma once

#include "storage/storage_defines.hpp"
#include "storage/index.hpp"

#include "base/buffer_vector.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/map.hpp"
#include "std/function.hpp"


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
    virtual void CountryStatusChanged(TGroup const & group, int position,
                                      TStatus const & oldStatus, TStatus const & newStatus) = 0;
    virtual void CountryOptionsChanged(TGroup const & group, int position,
                                       TMapOptions const & oldOpt, TMapOptions const & newOpt) = 0;
    virtual void DownloadingProgressUpdate(TGroup const & group, int position,
                                           LocalAndRemoteSizeT const & progress) = 0;
  };

  ActiveMapsLayout(Framework & framework);
  ~ActiveMapsLayout();

  size_t GetSizeToUpdateAllInBytes() const;
  void UpdateAll();
  void CancelAll();

  int GetOutOfDateCount() const;

  int GetCountInGroup(TGroup const & group) const;
  bool IsEmpty() const;
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
  LocalAndRemoteSizeT const GetRemoteCountrySizes(TGroup const & group, int position) const;
  LocalAndRemoteSizeT const GetRemoteCountrySizes(TIndex const & index) const;

  int AddListener(ActiveMapsListener * listener);
  void RemoveListener(int slotID);

  void DownloadMap(TIndex const & index, TMapOptions const & options);
  void DownloadMap(TGroup const & group, int position, TMapOptions const & options);
  void DeleteMap(TIndex const & index, TMapOptions const & options);
  void DeleteMap(TGroup const & group, int position, TMapOptions const & options);
  void RetryDownloading(TGroup const & group, int position);
  void RetryDownloading(TIndex const & index);

  ///@{ For CountryStatusDisplay only
  TIndex const & GetCoreIndex(TGroup const & group, int position) const;
  string const GetFormatedCountryName(TIndex const & index) const;
  ///@}

  bool IsDownloadingActive() const;
  void CancelDownloading(TGroup const & group, int position);

  void ShowMap(TGroup const & group, int position);

private:
  friend class CountryTree;
  Storage const & GetStorage() const;
  Storage & GetStorage();

  void Init(vector<string> const & maps);
  void Clear();

  void ShowMap(TIndex const & index);

  void StatusChangedCallback(TIndex const & index);
  void ProgressChangedCallback(TIndex const & index, LocalAndRemoteSizeT const & sizes);

  class Item
  {
    buffer_vector<TIndex, 1> m_index;
  public:
    template <class TCont> Item(TCont const & cont,
                                TStatus status,
                                TMapOptions options,
                                TMapOptions downloadRequest)
      : m_index(cont.begin(), cont.end()), m_status(status),
        m_options(options), m_downloadRequest(downloadRequest)
    {
      ASSERT(!m_index.empty(), ());
    }

    bool IsEqual(TIndex const & index) const;
    bool IsEqual(Item const & item) const;
    TIndex const & Index() const { return m_index[0]; }

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

  int InsertInGroup(TGroup const & group, Item const & item);
  void DeleteFromGroup(TGroup const & group, int position);
  int MoveItemToGroup(TGroup const & group, int position, TGroup const & newGroup);

  void NotifyInsertion(TGroup const & group, int position);
  void NotifyDeletion(TGroup const & group, int position);
  void NotifyMove(TGroup const & oldGroup, int oldPosition,
                  TGroup const & newGroup, int newPosition);

  void NotifyStatusChanged(TGroup const & group, int position,
                           TStatus const & oldStatus, TStatus const & newStatus);
  void NotifyOptionsChanged(TGroup const & group, int position,
                            TMapOptions const & oldOpt, TMapOptions const & newOpt);

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
