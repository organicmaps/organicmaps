#include "map/gps_track_collection.hpp"

#include "geometry/mercator.hpp"

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

GpsTrackCollection::GpsTrackCollection() : m_lastId(0) {}

std::pair<size_t, size_t> GpsTrackCollection::Add(std::vector<TItem> const & items)
{
  size_t startId = m_lastId;
  size_t added = 0;

  // Rollbacker ensure strong guarantee if exception happens while adding items
  Rollbacker<TItem> rollbacker(m_items);

  for (auto const & item : items)
  {
    ASSERT_GREATER_OR_EQUAL(item.m_timestamp, 0, ());
    if (!m_items.empty() && m_items.back().m_timestamp > item.m_timestamp)
      continue;

    m_items.emplace_back(item);
    ++added;
  }

  rollbacker.Reset();

  if (0 == added)
  {
    // Invalid timestamp order
    return std::make_pair(kInvalidId, kInvalidId);  // Nothing was added
  }

  // Feed only after deque insertion succeeds. The bounded processor does not allocate.
  for (size_t i = m_items.size() - added; i < m_items.size(); ++i)
  {
    auto const & item = m_items[i];
    auto const point = mercator::FromLatLon(item.m_latitude, item.m_longitude);
    if (i > 0)
      m_statistics.m_length += mercator::DistanceOnEarth(m_lastPoint, point);
    m_lastPoint = point;
    auto const altitude = static_cast<geometry::Altitude>(item.m_altitude);
    m_invalidAltitude |= altitude == geometry::kInvalidAltitude;
    m_hasAltitude |= altitude != geometry::kDefaultAltitudeMeters;
    if (!m_invalidAltitude)
      m_elevation.Add({m_statistics.m_length, altitude});
  }
  m_statistics.m_duration = m_items.back().m_timestamp - m_items.front().m_timestamp;
  m_statisticsDirty = true;
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
  m_elevation = {};
  m_lastPoint = {};
  m_statisticsDirty = false;
  m_invalidAltitude = false;
  m_hasAltitude = false;

  if (resetIds)
    m_lastId = 0;

  return res;
}

size_t GpsTrackCollection::GetSize() const
{
  return m_items.size();
}

TrackStatistics const & GpsTrackCollection::GetTrackStatistics() const
{
  if (m_statisticsDirty)
  {
    if (!m_invalidAltitude)
      m_elevation.GetTotals().Apply(m_statistics);
    else
    {
      m_statistics.m_ascent = m_statistics.m_descent = 0;
      m_statistics.m_minElevation = m_statistics.m_maxElevation = geometry::kDefaultAltitudeMeters;
    }
    m_statisticsDirty = false;
  }
  return m_statistics;
}

bool GpsTrackCollection::IsEmpty() const
{
  return m_items.size() < 2;
}
