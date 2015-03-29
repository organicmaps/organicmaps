#pragma once

#include "map/active_maps_layout.hpp"

#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

#include "platform/local_country_file.hpp"

#include "base/buffer_vector.hpp"

#include "std/function.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"


class Framework;

namespace storage
{

class Storage;
class CountryTree
{
public:
  class CountryTreeListener
  {
  public:
    virtual void ItemStatusChanged(int childPosition) = 0;
    virtual void ItemProgressChanged(int childPosition, LocalAndRemoteSizeT const & sizes) = 0;
  };

  CountryTree() = default;
  explicit CountryTree(shared_ptr<ActiveMapsLayout> activeMaps);

  CountryTree(CountryTree const & other) = delete;
  ~CountryTree();

  CountryTree & operator=(CountryTree const & other);

  ActiveMapsLayout & GetActiveMapLayout();
  ActiveMapsLayout const & GetActiveMapLayout() const;

  bool IsValid() const;

  void SetDefaultRoot();
  void SetParentAsRoot();
  void SetChildAsRoot(int childPosition);
  void ResetRoot();
  bool HasRoot() const;

  bool HasParent() const;

  int GetChildCount() const;
  bool IsLeaf(int childPosition) const;
  string const & GetChildName(int position) const;
  /// call this only if child IsLeaf == true
  TStatus GetLeafStatus(int position) const;
  MapOptions GetLeafOptions(int position) const;
  LocalAndRemoteSizeT const GetDownloadableLeafSize(int position) const;
  LocalAndRemoteSizeT const GetLeafSize(int position, MapOptions const & options) const;
  LocalAndRemoteSizeT const GetRemoteLeafSizes(int position) const;
  bool IsCountryRoot() const;
  string const & GetRootName() const;

  ///@{
  void DownloadCountry(int childPosition, MapOptions const & options);
  void DeleteCountry(int childPosition, MapOptions const & options);
  void RetryDownloading(int childPosition);
  ///@}
  void CancelDownloading(int childPosition);

  void SetListener(CountryTreeListener * listener);
  void ResetListener();

  void ShowLeafOnMap(int position);

private:
  Storage const & GetStorage() const;
  Storage & GetStorage();
  void ConnectToCoreStorage();
  void DisconnectFromCoreStorage();

private:
  TIndex const & GetCurrentRoot() const;
  void SetRoot(TIndex index);
  TIndex const & GetChild(int childPosition) const;
  int GetChildPosition(TIndex const & index);

  void NotifyStatusChanged(TIndex const & index);
  void NotifyProgressChanged(TIndex const & index, LocalAndRemoteSizeT const & sizes);

private:
  int m_subscribeSlotID = 0;
  shared_ptr<ActiveMapsLayout> m_layout;

  buffer_vector<TIndex, 16> m_levelItems;

  CountryTreeListener * m_listener = nullptr;
};

}
