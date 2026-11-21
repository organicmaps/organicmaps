#include "map/gps_track_collection.hpp"

#include "kml/type_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

bool GpsTrackCollection::CanAdd(TItem const & item) const
{
  if (m_items.empty())
    return true;

  auto const & last = m_items.back();
  return item.m_timestamp >= last.m_timestamp &&
         !kml::IsSameTrackPoint(mercator::FromLatLon(last.GetLatLon()), mercator::FromLatLon(item.GetLatLon()));
}

std::pair<size_t, size_t> GpsTrackCollection::Add(std::vector<TItem> const & items)
{
  auto const startId = m_lastId;

  for (auto const & item : items)
  {
    if (!CanAdd(item))
      continue;
    m_items.emplace_back(item);
    m_statistics.AddGpsInfoPoint(item);
    ++m_lastId;
  }

  if (m_lastId == startId)
    return {kInvalidId, kInvalidId};
  return {startId, m_lastId - 1};
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
  m_elevationInfo.Clear();

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
  if (!m_statistics.HasGpsElevationProfile())
  {
    m_elevationInfo.Clear();
    return m_elevationInfo;
  }

  for (auto it = m_items.begin() + m_elevationInfo.GetSize(); it != m_items.end(); ++it)
    m_elevationInfo.AddGpsPoint(*it);
  return m_elevationInfo;
}
