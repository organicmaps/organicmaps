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
  HotelStatus result = Get(m_notReadyHotels, hotelId);

  if (result == HotelStatus::Absent)
    result = Get(m_hotelToStatus, hotelId);

  return result;
}

void Cache::Reserve(std::string const & hotelId)
{
  ASSERT(m_hotelToStatus.find(hotelId) == m_hotelToStatus.end(), ());

  m_notReadyHotels.emplace(hotelId, Item());
}

void Cache::Insert(std::string const & hotelId, HotelStatus const s)
{
  ASSERT_NOT_EQUAL(s, HotelStatus::NotReady,
                   ("Please, use Cache::Reserve method for HotelStatus::NotReady"));

  RemoveExtra();

  Item item(s);
  m_hotelToStatus[hotelId] = std::move(item);
  m_notReadyHotels.erase(hotelId);
}

void Cache::RemoveOutdated()
{
  if (m_expiryPeriodSeconds == 0)
    return;

  RemoveOutdated(m_hotelToStatus);
  RemoveOutdated(m_notReadyHotels);
}

void Cache::Clear()
{
  m_hotelToStatus.clear();
  m_notReadyHotels.clear();
}

void Cache::RemoveExtra()
{
  if (m_maxCount == 0 || m_hotelToStatus.size() < m_maxCount)
    return;

  m_hotelToStatus.clear();
}

bool Cache::IsExpired(Clock::time_point const & timestamp) const
{
  return Clock::now() > timestamp + seconds(m_expiryPeriodSeconds);
}

Cache::HotelStatus Cache::Get(HotelsMap & src, std::string const & hotelId)
{
  auto const it = src.find(hotelId);

  if (it == src.cend())
    return HotelStatus::Absent;

  if (m_expiryPeriodSeconds != 0 && IsExpired(it->second.m_timestamp))
  {
    src.erase(it);
    return HotelStatus::Absent;
  }

  return it->second.m_status;
}

void Cache::RemoveOutdated(HotelsMap & src)
{
  for (auto it = src.begin(); it != src.end();)
  {
    if (IsExpired(it->second.m_timestamp))
      it = src.erase(it);
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
  UNREACHABLE();
}
}  // namespace availability
}  // namespace filter
}  // namespace booking
