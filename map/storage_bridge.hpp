#pragma once

#include "active_maps_layout.hpp"

#include "storage/index.hpp"
#include "storage/storage_defines.hpp"

#include "std/shared_ptr.hpp"

class StorageBridge : public storage::ActiveMapsLayout::ActiveMapsListener
{
public:
  using TOnChangedHandler = function<void(storage::TIndex const & /*countryIndex*/)>;

  StorageBridge(shared_ptr<storage::ActiveMapsLayout> activeMaps, TOnChangedHandler const & handler);
  ~StorageBridge() override;

  void CountryGroupChanged(storage::ActiveMapsLayout::TGroup const & oldGroup, int oldPosition,
                           storage::ActiveMapsLayout::TGroup const & newGroup, int newPosition) override;
  void CountryStatusChanged(storage::ActiveMapsLayout::TGroup const & group, int position,
                            storage::TStatus const & oldStatus, storage::TStatus const & newStatus) override;
  void CountryOptionsChanged(storage::ActiveMapsLayout::TGroup const & group, int position,
                             storage::TMapOptions const & oldOpt, storage::TMapOptions const & newOpt) override;
  void DownloadingProgressUpdate(storage::ActiveMapsLayout::TGroup const & group, int position,
                                 storage::LocalAndRemoteSizeT const & progress) override;

private:
  shared_ptr<storage::ActiveMapsLayout> m_activeMaps;
  TOnChangedHandler m_handler;
  int m_slot;

  void ReportChanges(storage::ActiveMapsLayout::TGroup const & group, int position);
};

