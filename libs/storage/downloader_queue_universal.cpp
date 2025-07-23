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
  m_queue.emplace_back(std::move(country));
  m_queue.back().OnCountryInQueue();
}

void Queue::Clear()
{
  m_queue.clear();
}
}  // namespace storage
