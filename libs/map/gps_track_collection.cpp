#include "map/gps_track_collection.hpp"

#include "base/assert.hpp"

namespace
{

// Simple rollbacker which restores deque state
template <typename T>
class Rollbacker
{
public:
  Rollbacker(std::deque<T> & cont) : m_cont(&cont), m_size(cont.size()) {}
  ~Rollbacker()
  {
    if (m_cont && m_cont->size() > m_size)
      m_cont->erase(m_cont->begin() + m_size, m_cont->end());
  }
  void Reset() { m_cont = nullptr; }

private:
  std::deque<T> * m_cont;
  size_t const m_size;
};

}  // namespace

GpsTrackCollection::GpsTrackCollection() : m_lastId(0), m_elevationInfoDirty(true) {}

std::pair<size_t, size_t> GpsTrackCollection::Add(std::vector<TItem> const & items)
{
  size_t startId = m_lastId;
  size_t added = 0;

  // Rollbacker ensure strong guarantee if exception happens while adding items
  Rollbacker<TItem> rollbacker(m_items);

  for (auto const & item : items)
  {
    if (!m_items.empty() && m_items.back().m_timestamp > item.m_timestamp)
      continue;

    m_statistics.AddGpsInfoPoint(item);

    m_items.emplace_back(item);
    ++added;
  }

  m_elevationInfoDirty = true;

  rollbacker.Reset();

  if (0 == added)
  {
    // Invalid timestamp order
    return std::make_pair(kInvalidId, kInvalidId);  // Nothing was added
  }

  m_lastId += added;

  return std::make_pair(startId, startId + added - 1);
}

std::pair<size_t, size_t> GpsTrackCollection::Clear(bool resetIds)
{
  if (m_items.empty())
  {
    if (resetIds)
      m_lastId = 0;

    return std::make_pair(kInvalidId, kInvalidId);
  }

  ASSERT_GREATER_OR_EQUAL(m_lastId, m_items.size(), ());

  // Range of evicted items
  auto const res = std::make_pair(m_lastId - m_items.size(), m_lastId - 1);

  m_items.clear();
  m_items.shrink_to_fit();
  m_statistics = {};
  m_elevationInfo = {};

  if (resetIds)
    m_lastId = 0;

  return res;
}

size_t GpsTrackCollection::GetSize() const
{
  return m_items.size();
}

ElevationInfo const & GpsTrackCollection::UpdateAndGetElevationInfo()
{
  if (!m_elevationInfoDirty)
    return m_elevationInfo;

  auto const elevationInfoSize = m_elevationInfo.GetSize();
  if (elevationInfoSize < m_items.size())
  {
    std::vector<TItem> const missedPoints(m_items.begin() + elevationInfoSize, m_items.end());
    m_elevationInfo.AddGpsPoints(missedPoints);
  }
  m_elevationInfoDirty = false;
  return m_elevationInfo;
}

bool GpsTrackCollection::IsEmpty() const
{
  return m_items.size() < 2;
}
