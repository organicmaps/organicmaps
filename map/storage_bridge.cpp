#include "storage_bridge.hpp"

using namespace storage;

StorageBridge::StorageBridge(shared_ptr<storage::ActiveMapsLayout> activeMaps)
  : m_activeMaps(activeMaps)
{
}

string StorageBridge::GetCurrentCountryName() const
{
  ASSERT(m_currentIndex != TIndex(), ());
  return m_activeMaps->GetFormatedCountryName(m_currentIndex);
}

size_t StorageBridge::GetMapSize() const
{
  ASSERT(m_currentIndex != TIndex(), ());
  return m_activeMaps->GetRemoteCountrySizes(m_currentIndex).first;
}

size_t StorageBridge::GetRoutingSize() const
{
  ASSERT(m_currentIndex != TIndex(), ());
  return m_activeMaps->GetRemoteCountrySizes(m_currentIndex).second;
}

size_t StorageBridge::GetDownloadProgress() const
{
  if (m_progress.second == 0)
    return 0;

  return m_progress.first * 100 / m_progress.second;
}

void StorageBridge::SetCountryIndex(storage::TIndex const & index)
{
  m_currentIndex = index;
}

storage::TIndex StorageBridge::GetCountryIndex() const
{
  return m_currentIndex;
}

storage::TStatus StorageBridge::GetCountryStatus() const
{
  if (m_currentIndex == TIndex())
    return TStatus::EOnDisk;

  return m_activeMaps->GetCountryStatus(m_currentIndex);
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
  if (m_activeMaps->GetCoreIndex(group, position) == m_currentIndex && m_statusChanged)
    m_statusChanged();
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
  if (m_activeMaps->GetCoreIndex(group, position) == m_currentIndex)
    m_progress = progress;
}
