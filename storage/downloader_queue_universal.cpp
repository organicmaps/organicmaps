#include "storage/downloader_queue_universal.hpp"

#include <algorithm>

namespace storage
{
bool Queue::IsEmpty() const
{
  return m_queue.empty();
}

bool Queue::Contains(CountryId const & country) const
{
  return std::find(m_queue.cbegin(), m_queue.cend(), country) != m_queue.cend();
}

void Queue::ForEachCountry(ForEachCountryFunction const & fn) const
{
  for (auto const & queuedCountry : m_queue)
  {
    fn(queuedCountry);
  }
}

void Queue::ForEachCountry(ForEachCountryMutable const & fn)
{
  for (auto & queuedCountry : m_queue)
  {
    fn(queuedCountry);
  }
}

CountryId const & Queue::GetFirstId() const
{
  CHECK(!m_queue.empty(), ());

  return m_queue.front().GetCountryId();
}

QueuedCountry & Queue::GetFirstCountry()
{
  CHECK(!m_queue.empty(), ());

  return m_queue.front();
}

void Queue::Remove(const storage::CountryId & id)
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

void Queue::Clear()
{
  m_queue.clear();
}

size_t Queue::Count() const
{
  return m_queue.size();
}
}  // namespace storage

