#pragma once

#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file.hpp"

#include "base/buffer_vector.hpp"

#include "std/function.hpp"
#include "std/map.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"


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
                                       MapOptions const & oldOpt, MapOptions const & newOpt) = 0;
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
  MapOptions GetCountryOptions(TGroup const & group, int position) const;
  MapOptions GetCountryOptions(storage::TIndex const & index) const;
  LocalAndRemoteSizeT const GetDownloadableCountrySize(TGroup const & group, int position) const;
  LocalAndRemoteSizeT const GetDownloadableCountrySize(TIndex const & index) const;
  LocalAndRemoteSizeT const GetCountrySize(TGroup const & group, int position, MapOptions const & options) const;
  LocalAndRemoteSizeT const GetCountrySize(TIndex const & index, MapOptions const & options) const;
  LocalAndRemoteSizeT const GetRemoteCountrySizes(TGroup const & group, int position) const;
  LocalAndRemoteSizeT const GetRemoteCountrySizes(TIndex const & index) const;

  int AddListener(ActiveMapsListener * listener);
  void RemoveListener(int slotID);

  void DownloadMap(TIndex const & index, MapOptions const & options);
  void DownloadMap(TGroup const & group, int position, MapOptions const & options);
  void DeleteMap(TIndex const & index, MapOptions const & options);
  void DeleteMap(TGroup const & group, int position, MapOptions const & options);
  void RetryDownloading(TGroup const & group, int position);
  void RetryDownloading(TIndex const & index);

  ///@{ For CountryStatus only
  TIndex const & GetCoreIndex(TGroup const & group, int position) const;
  string const GetFormatedCountryName(TIndex const & index) const;
  ///@}

  bool IsDownloadingActive() const;
  void CancelDownloading(TGroup const & group, int position);
  void CancelDownloading(TIndex const & index);

  TIndex GetCurrentDownloadingCountryIndex() const;

  void ShowMap(TGroup const & group, int position);

  /// @param[in]  Sorted vector of current .mwm files.
  using TLocalFilePtr = shared_ptr<platform::LocalCountryFile>;
  void Init(vector<TLocalFilePtr> const & files);
  void Clear();

private:
  friend class CountryTree;
  Storage const & GetStorage() const;
  Storage & GetStorage();

  void ShowMap(TIndex const & index);

  void StatusChangedCallback(TIndex const & index);
  void ProgressChangedCallback(TIndex const & index, LocalAndRemoteSizeT const & sizes);

  class Item
  {
    /// Usually, this vector has size = 1 and sometimes = 2.
    buffer_vector<TIndex, 1> m_indexes;

  public:
    template <class TCont> Item(TCont const & cont,
                                TStatus status,
                                MapOptions options,
                                MapOptions downloadRequest)
      : m_indexes(cont.begin(), cont.end()), m_status(status),
        m_options(options), m_downloadRequest(downloadRequest)
    {
      ASSERT(!m_indexes.empty(), ());
    }

    /// Use this functions to compare Items by input TIndex.
    //@{
    bool IsEqual(TIndex const & index) const;
    bool IsEqual(Item const & item) const;
    //@}

    /// Get any key TIndex for the correspondent Item.
    TIndex const & Index() const { return m_indexes[0]; }

    TStatus m_status;
    MapOptions m_options;
    MapOptions m_downloadRequest;
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
                            MapOptions const & oldOpt, MapOptions const & newOpt);

  MapOptions ValidOptionsForDownload(MapOptions const & options);
  MapOptions ValidOptionsForDelete(MapOptions const & options);

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
