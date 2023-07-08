#include "map/gps_track_collection.hpp"

#include "base/assert.hpp"

#include <algorithm>

using namespace std;
using namespace std::chrono;

namespace
{

size_t const kSecondsPerHour = 60 * 60;
size_t const kLinearSearchCount = 10;

// Simple rollbacker which restores deque state
template <typename T>
class Rollbacker
{
public:
  Rollbacker(deque<T> & cont)
    : m_cont(&cont)
    , m_size(cont.size())
  {}
  ~Rollbacker()
  {
    if (m_cont && m_cont->size() > m_size)
      m_cont->erase(m_cont->begin() + m_size, m_cont->end());
  }
  void Reset() { m_cont = nullptr; }
private:
  deque<T> * m_cont;
  size_t const m_size;
};

} // namespace

size_t const GpsTrackCollection::kInvalidId = numeric_limits<size_t>::max();

GpsTrackCollection::GpsTrackCollection(size_t maxSize, hours duration)
  : m_maxSize(maxSize)
  , m_duration(duration)
  , m_lastId(0)
{
}

size_t GpsTrackCollection::Add(TItem const & item, pair<size_t, size_t> & evictedIds)
{
  if (!m_items.empty() && m_items.back().m_timestamp > item.m_timestamp)
  {
    // Invalid timestamp order
    evictedIds = make_pair(kInvalidId, kInvalidId); // Nothing was evicted
    return kInvalidId; // Nothing was added
  }

  m_items.emplace_back(item);
  ++m_lastId;

  evictedIds = RemoveExtraItems();

  return m_lastId - 1;
}

pair<size_t, size_t> GpsTrackCollection::Add(vector<TItem> const & items, pair<size_t, size_t> & evictedIds)
{
  size_t startId = m_lastId;
  size_t added = 0;

  // Rollbacker ensure strong guarantee if exception happens while adding items
  Rollbacker<TItem> rollbacker(m_items);

  for (auto const & item : items)
  {
    if (!m_items.empty() && m_items.back().m_timestamp > item.m_timestamp)
      continue;

    m_items.emplace_back(item);
    ++added;
  }

  rollbacker.Reset();

  if (0 == added)
  {
    // Invalid timestamp order
    evictedIds = make_pair(kInvalidId, kInvalidId); // Nothing was evicted
    return make_pair(kInvalidId, kInvalidId); // Nothing was added
  }

  m_lastId += added;

  evictedIds = RemoveExtraItems();

  return make_pair(startId, startId + added - 1);
}

hours GpsTrackCollection::GetDuration() const
{
  return m_duration;
}

pair<size_t, size_t> GpsTrackCollection::SetDuration(hours duration)
{
  m_duration = duration;

  if (m_items.empty())
    return make_pair(kInvalidId, kInvalidId);

  return RemoveExtraItems();
}

pair<size_t, size_t> GpsTrackCollection::Clear(bool resetIds)
{
  if (m_items.empty())
  {
    if (resetIds)
      m_lastId = 0;

    return make_pair(kInvalidId, kInvalidId);
  }

  ASSERT_GREATER_OR_EQUAL(m_lastId, m_items.size(), ());

  // Range of evicted items
  auto const res = make_pair(m_lastId - m_items.size(), m_lastId - 1);

  m_items.clear();
  m_items.shrink_to_fit();

  if (resetIds)
    m_lastId = 0;

  return res;
}

size_t GpsTrackCollection::GetSize() const
{
  return m_items.size();
}

size_t GpsTrackCollection::GetMaxSize() const
{
  return m_maxSize;
}

bool GpsTrackCollection::IsEmpty() const
{
  return m_items.empty();
}

pair<double, double> GpsTrackCollection::GetTimestampRange() const
{
  if (m_items.empty())
    return make_pair(0, 0);

  ASSERT_LESS_OR_EQUAL(m_items.front().m_timestamp, m_items.back().m_timestamp, ());

  return make_pair(m_items.front().m_timestamp, m_items.back().m_timestamp);
}

pair<size_t, size_t> GpsTrackCollection::RemoveUntil(deque<TItem>::iterator i)
{
  auto const res = make_pair(m_lastId - m_items.size(),
                             m_lastId - m_items.size() + distance(m_items.begin(), i) - 1);
  m_items.erase(m_items.begin(), i);
  return res;
}

pair<size_t, size_t> GpsTrackCollection::RemoveExtraItems()
{
  if (m_items.empty())
    return make_pair(kInvalidId, kInvalidId); // Nothing to remove

  double const lowerBound = m_items.back().m_timestamp - m_duration.count() * kSecondsPerHour;

  ASSERT_GREATER_OR_EQUAL(m_lastId, m_items.size(), ());

  if (m_items.front().m_timestamp >= lowerBound)
  {
    // All items lie on right side of lower bound,
    // but need to remove items by size.
    if (m_items.size() <= m_maxSize)
      return make_pair(kInvalidId, kInvalidId); // Nothing to remove, all points survived.
    return RemoveUntil(m_items.begin() + m_items.size() - m_maxSize);
  }

  if (m_items.back().m_timestamp <= lowerBound)
  {
    // All items lie on left side of lower bound. Remove all items.
    return RemoveUntil(m_items.end());
  }

  bool found = false;
  auto i = m_items.begin();

  // First, try linear search for short distance. It is common case for sliding window
  // when new items will evict old items.
  for (size_t j = 0; j < kLinearSearchCount; ++i, ++j)
  {
    ASSERT(i != m_items.end(), ());

    if (i->m_timestamp > lowerBound)
    {
      found = true;
      break;
    }
  }

  // If item wasn't found by linear search, since m_items are sorted by timestamp, use lower_bound to find bound
  if (!found)
  {
    TItem t;
    t.m_timestamp = lowerBound;
    i = lower_bound(i, m_items.end(), t, [](TItem const & a, TItem const & b)->bool{ return a.m_timestamp < b.m_timestamp; });

    ASSERT(i != m_items.end(), ());
  }

  // If remaining part has size more than max size then cut off to leave max size
  size_t const remains = distance(i, m_items.end());
  if (remains > m_maxSize)
    i += remains - m_maxSize;

  return RemoveUntil(i);
}
