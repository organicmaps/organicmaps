#include "map/booking_filter_cache.hpp"

using namespace std::chrono;

namespace booking
{
namespace filter
{
namespace availability
{
Cache::Cache(size_t maxCount, size_t expiryPeriodSeconds)
  : m_maxCount(maxCount), m_expiryPeriodSeconds(expiryPeriodSeconds)
{
}

Cache::HotelStatus Cache::Get(std::string const & hotelId)
{
  auto const it = m_hotelToResult.find(hotelId);

  if (it == m_hotelToResult.cend())
    return HotelStatus::Absent;

  if (m_expiryPeriodSeconds != 0)
  {
    auto const timeDiff = Clock::now() - it->second.m_timestamp;
    if (timeDiff > seconds(m_expiryPeriodSeconds))
    {
      m_hotelToResult.erase(it);
      return HotelStatus::Absent;
    }
  }

  return it->second.m_status;
}

void Cache::Reserve(std::string const & hotelId)
{
  Item item;
  m_hotelToResult.emplace(hotelId, std::move(item));
}

void Cache::Insert(std::string const & hotelId, HotelStatus const s)
{
  RemoveExtra();

  Item item(s);
  m_hotelToResult[hotelId] = std::move(item);
}

void Cache::RemoveOutdated()
{
  if (m_expiryPeriodSeconds == 0)
    return;

  for (auto it = m_hotelToResult.begin(); it != m_hotelToResult.end();)
  {
    auto const timeDiff = Clock::now() - it->second.m_timestamp;
    if (timeDiff > seconds(m_expiryPeriodSeconds))
      it = m_hotelToResult.erase(it);
    else
      ++it;
  }
}

void Cache::Drop()
{
  m_hotelToResult.clear();
}

void Cache::RemoveExtra()
{
  if (m_maxCount == 0 || m_hotelToResult.size() < m_maxCount)
    return;

  for (auto it = m_hotelToResult.begin(); it != m_hotelToResult.end();)
  {
    if (it->second.m_status != HotelStatus::NotReady)
      it = m_hotelToResult.erase(it);
    else
      ++it;
  }
}

std::string DebugPrint(Cache::HotelStatus status)
{
  switch (status)
  {
  case Cache::HotelStatus::Absent: return "Absent";
  case Cache::HotelStatus::NotReady: return "NotReady";
  case Cache::HotelStatus::Unavailable: return "Unavailable";
  case Cache::HotelStatus::Available: return "Available";
  }
}
}  // namespace availability
}  // namespace filter
}  // namespace booking
