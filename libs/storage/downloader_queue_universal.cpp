#include "storage/downloader_queue_universal.hpp"

#include <algorithm>

namespace storage
{
bool Queue::IsEmpty() const
{
  return m_queue.empty();
}

size_t Queue::Count() const
{
  return m_queue.size();
}

bool Queue::Contains(CountryId const & country) const
{
  return base::IsExist(m_queue, country);
}

void Queue::ForEachCountry(ForEachCountryFunction const & fn) const
{
  for (auto const & queuedCountry : m_queue)
    fn(queuedCountry);
}

void Queue::ForEachCountry(ForEachCountryMutable const & fn)
{
  for (auto & queuedCountry : m_queue)
    fn(queuedCountry);
}

CountryId const & Queue::GetFirstId() const
{
  CHECK(!m_queue.empty(), ());

  return m_queue.front().GetCountryId();
}

QueuedCountry const & Queue::GetFirstCountry() const
{
  CHECK(!m_queue.empty(), ());

  return m_queue.front();
}

void Queue::Remove(storage::CountryId const & id)
{
  auto it = std::find(m_queue.begin(), m_queue.end(), id);
  if (it != m_queue.end())
    m_queue.erase(it);
}

void Queue::PopFront()
{
  CHECK(!m_queue.empty(), ());

  m_queue.pop_front();
}

void Queue::Append(QueuedCountry && country)
{
  auto pos = m_queue.end();
  // A map (e.g. for a route) shouldn't wait behind the large terrain blocks. The front is in progress.
  if (country.GetFileType() != MapFileType::Terrain && !m_queue.empty())
  {
    pos = std::find_if(std::next(m_queue.begin()), m_queue.end(),
                       [](QueuedCountry const & queued) { return queued.GetFileType() == MapFileType::Terrain; });
  }
  m_queue.insert(pos, std::move(country))->OnCountryInQueue();
}

void Queue::Clear()
{
  m_queue.clear();
}
}  // namespace storage
