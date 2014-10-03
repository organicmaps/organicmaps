#pragma once

#include "active_maps_layout.hpp"

#include "../storage/index.hpp"
#include "../storage/storage_defines.hpp"

#include "../base/buffer_vector.hpp"

#include "../std/string.hpp"
#include "../std/function.hpp"

class Framework;

class CountryTree
{
public:
  CountryTree(Framework * framework);
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
  storage::TStatus GetLeafStatus(int position) const;
  storage::TMapOptions GetLeafOptions(int position) const;

  ///@{
  void DownloadCountry(int childPosition, storage::TMapOptions const & options);
  void DeleteCountry(int childPosition, storage::TMapOptions const & options);
  ///@}
  void CancelDownloading(int childPosition);

  /// int - child position for current root
  typedef function<void (int)> TItemChangedFn;
  void SetItemChangedListener(TItemChangedFn const & callback);

  /// int - child position for current root
  /// LocalAndRemoteSizeT - progress change info
  typedef function<void (int, storage::LocalAndRemoteSizeT const &)> TItemProgressChangedFn;
  void SetItemProgressListener(TItemProgressChangedFn const & callback);

private:
  storage::TIndex const & GetCurrentRoot() const;
  void SetRoot(storage::TIndex const & index);
  storage::TIndex const & GetChild(int childPosition) const;
  int GetChildPosition(storage::TIndex const & index);

private:
  Framework * m_framework = nullptr;
  int m_subscribeSlotID = 0;
  ActiveMapsLayout m_layout;

  buffer_vector<storage::TIndex, 16> m_levelItems;

  TItemChangedFn m_itemCallback;
  TItemProgressChangedFn m_progressCallback;
};
