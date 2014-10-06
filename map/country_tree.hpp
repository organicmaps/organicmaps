#pragma once

#include "active_maps_layout.hpp"

#include "../storage/index.hpp"
#include "../storage/storage_defines.hpp"

#include "../base/buffer_vector.hpp"

#include "../std/string.hpp"
#include "../std/function.hpp"

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

  CountryTree(Framework & framework);
  ~CountryTree();

  ActiveMapsLayout & GetActiveMapLayout();

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
  TMapOptions GetLeafOptions(int position) const;

  ///@{
  void DownloadCountry(int childPosition, TMapOptions const & options);
  void DeleteCountry(int childPosition, TMapOptions const & options);
  ///@}
  void CancelDownloading(int childPosition);

  void SetListener(CountryTreeListener * listener);
  void ResetListener();

private:
  Storage const & GetStorage() const;
  Storage & GetStorage();

private:
  TIndex const & GetCurrentRoot() const;
  void SetRoot(TIndex const & index);
  TIndex const & GetChild(int childPosition) const;
  int GetChildPosition(TIndex const & index);

  void NotifyStatusChanged(TIndex const & index);
  void NotifyProgressChanged(TIndex const & index, LocalAndRemoteSizeT const & sizes);

private:
  int m_subscribeSlotID = 0;
  ActiveMapsLayout m_layout;

  buffer_vector<TIndex, 16> m_levelItems;

  CountryTreeListener * m_listener = nullptr;
};

}
