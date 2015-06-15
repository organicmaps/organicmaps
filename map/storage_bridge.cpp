#include "storage_bridge.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"

using namespace storage;

StorageBridge::StorageBridge(shared_ptr<storage::ActiveMapsLayout> activeMaps, TOnChangedHandler const & handler)
  : m_activeMaps(activeMaps)
  , m_handler(handler)
{
  ASSERT(m_activeMaps != nullptr, ());
  m_slot = m_activeMaps->AddListener(this);
}

StorageBridge::~StorageBridge()
{
  m_activeMaps->RemoveListener(m_slot);
}

void StorageBridge::CountryGroupChanged(ActiveMapsLayout::TGroup const & oldGroup, int oldPosition,
                                        ActiveMapsLayout::TGroup const & newGroup, int newPosition)
{
  UNUSED_VALUE(oldGroup);
  UNUSED_VALUE(oldPosition);
  UNUSED_VALUE(newGroup);
  UNUSED_VALUE(newPosition);
}

void StorageBridge::CountryStatusChanged(ActiveMapsLayout::TGroup const & group, int position,
                                         TStatus const & oldStatus, TStatus const & newStatus)
{
  UNUSED_VALUE(oldStatus);
  UNUSED_VALUE(newStatus);
  ReportChanges(group, position);
}

void StorageBridge::CountryOptionsChanged(ActiveMapsLayout::TGroup const & group, int position,
                                          TMapOptions const & oldOpt, TMapOptions const & newOpt)
{
  UNUSED_VALUE(group);
  UNUSED_VALUE(position);
  UNUSED_VALUE(oldOpt);
  UNUSED_VALUE(newOpt);
}

void StorageBridge::DownloadingProgressUpdate(ActiveMapsLayout::TGroup const & group, int position,
                                              LocalAndRemoteSizeT const & progress)
{
  UNUSED_VALUE(progress);
  ReportChanges(group, position);
}

void StorageBridge::ReportChanges(ActiveMapsLayout::TGroup const & group, int position)
{
  storage::TIndex countryIndex = m_activeMaps->GetCoreIndex(group, position);

  if (m_handler != nullptr)
    m_handler(countryIndex);
}
