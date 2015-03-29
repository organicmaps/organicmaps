#pragma once

#include "active_maps_layout.hpp"

#include "../drape_gui/drape_gui.hpp"

#include "../storage/index.hpp"
#include "../storage/storage_defines.hpp"

class StorageBridge : public gui::StorageAccessor
                    , public storage::ActiveMapsLayout::ActiveMapsListener
{
public:
  StorageBridge(shared_ptr<storage::ActiveMapsLayout> activeMaps);

  string GetCurrentCountryName() const override;
  size_t GetMapSize() const override;
  size_t GetRoutingSize() const override;
  size_t GetDownloadProgress() const override;

  void SetCountryIndex(storage::TIndex const & index) override;
  storage::TIndex GetCountryIndex() const override;
  storage::TStatus GetCountryStatus() const override;

  void CountryGroupChanged(storage::ActiveMapsLayout::TGroup const & oldGroup, int oldPosition,
                           storage::ActiveMapsLayout::TGroup const & newGroup, int newPosition) override;
  void CountryStatusChanged(storage::ActiveMapsLayout::TGroup const & group, int position,
                            storage::TStatus const & oldStatus, storage::TStatus const & newStatus) override;
  void CountryOptionsChanged(storage::ActiveMapsLayout::TGroup const & group, int position,
                             storage::TMapOptions const & oldOpt, storage::TMapOptions const & newOpt) override;
  void DownloadingProgressUpdate(storage::ActiveMapsLayout::TGroup const & group, int position,
                                 storage::LocalAndRemoteSizeT const & progress) override;

private:
  storage::TIndex m_currentIndex;
  shared_ptr<storage::ActiveMapsLayout> m_activeMaps;
  storage::LocalAndRemoteSizeT m_progress;
};
