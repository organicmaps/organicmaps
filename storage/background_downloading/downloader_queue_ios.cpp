#import "storage/background_downloading/downloader_queue_ios.hpp"

#include <algorithm>

namespace storage
{
bool BackgroundDownloaderQueue::IsEmpty() const
{
  return m_queue.empty();
}

bool BackgroundDownloaderQueue::Contains(CountryId const & country) const
{
  return m_queue.find(country) != m_queue.cend();
}

void BackgroundDownloaderQueue::ForEachCountry(ForEachCountryFunction const & fn) const
{
  for (auto const & item : m_queue)
  {
    fn(item.second.m_queuedCountry);
  }
}

void BackgroundDownloaderQueue::SetTaskIdForCountryId(CountryId const & country, uint64_t taskId)
{
  auto const it = m_queue.find(country);
  CHECK(it != m_queue.cend(), ());

  it->second.m_taskId = taskId;
}

std::optional<uint64_t> BackgroundDownloaderQueue::GetTaskIdByCountryId(CountryId const & country) const
{
  auto const it = m_queue.find(country);
  if (it == m_queue.cend())
    return {};

  return it->second.m_taskId;
}

QueuedCountry & BackgroundDownloaderQueue::GetCountryById(CountryId const & countryId)
{
  return m_queue.at(countryId).m_queuedCountry;
}

void BackgroundDownloaderQueue::Remove(CountryId const & countryId)
{
  m_queue.erase(countryId);
}

void BackgroundDownloaderQueue::Clear()
{
  m_queue.clear();
}
}  // namespace storage
