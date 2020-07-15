#include "map/booking_filter_cache.hpp"

using namespace std::chrono;

namespace booking
{
namespace filter
{
namespace availability
{
namespace
{
bool IsExpired(Cache::Clock::time_point const & timestamp, size_t expiryPeriod)
{
  return Cache::Clock::now() > timestamp + seconds(expiryPeriod);
}

template <typename Item>
bool IsExpired(Item const & item, size_t expiryPeriod)
{
  return IsExpired(item.m_timestamp, expiryPeriod);
}

template <typename MapType>
typename MapType::const_iterator GetOrRemove(MapType & src, std::string const & hotelId,
                                             size_t expiryPeriod)
{
  auto const it = src.find(hotelId);

  if (it == src.cend())
    return src.cend();

  if (expiryPeriod != 0 && IsExpired(it->second, expiryPeriod))
  {
    src.erase(it);
    return src.cend();
  }

  return it;
}

template <typename MapType, typename Pred>
void Remove(MapType & src, Pred const & pred)
{
  for (auto it = src.begin(); it != src.end();)
  {
    if (pred(it->second))
      it = src.erase(it);
    else
      ++it;
  }
}
}  // namespace

Cache::Cache(size_t maxCount, size_t expiryPeriodSeconds)
  : m_maxCount(maxCount), m_expiryPeriodSeconds(expiryPeriodSeconds)
{
}

Cache::Info Cache::Get(std::string const & hotelId)
{
  auto const notReadyIt = GetOrRemove(m_notReadyHotels, hotelId, m_expiryPeriodSeconds);

  if (notReadyIt != m_notReadyHotels.cend())
    return Info(HotelStatus::NotReady);

  auto const unavailableIt = GetOrRemove(m_unavailableHotels, hotelId, m_expiryPeriodSeconds);

  if (unavailableIt != m_unavailableHotels.cend())
    return Info(HotelStatus::Unavailable);

  auto const availableIt = GetOrRemove(m_availableHotels, hotelId, m_expiryPeriodSeconds);

  if (availableIt != m_availableHotels.cend())
    return Info(HotelStatus::Available, availableIt->second.m_extras);

  return Info(HotelStatus::Absent);
}

void Cache::ReserveAdditional(size_t count)
{
  ReserveAdditional(m_unavailableHotels, count);
  ReserveAdditional(m_availableHotels, count);
}

void Cache::InsertNotReady(std::string const & hotelId)
{
  ASSERT(m_unavailableHotels.find(hotelId) == m_unavailableHotels.end(), ());
  ASSERT(m_availableHotels.find(hotelId) == m_availableHotels.end(), ());

  m_notReadyHotels.emplace(hotelId, Clock::now());
}

void Cache::InsertUnavailable(std::string const & hotelId)
{
  ReserveAdditional(m_unavailableHotels, 1);

  m_unavailableHotels[hotelId] = Clock::now();
  m_notReadyHotels.erase(hotelId);
  m_availableHotels.erase(hotelId);
}

void Cache::InsertAvailable(std::string const & hotelId, Extras && extras)
{
  ReserveAdditional(m_availableHotels, 1);

  m_availableHotels[hotelId] = Item(std::move(extras));
  m_notReadyHotels.erase(hotelId);
  m_unavailableHotels.erase(hotelId);
}

void Cache::RemoveOutdated()
{
  if (m_expiryPeriodSeconds == 0)
    return;

  Remove(m_notReadyHotels, [this](auto const & v) { return IsExpired(v, m_expiryPeriodSeconds); });
  Remove(m_unavailableHotels,
         [this](auto const & v) { return IsExpired(v, m_expiryPeriodSeconds); });
  Remove(m_availableHotels,
         [this](auto const & v) { return IsExpired(v.m_timestamp, m_expiryPeriodSeconds); });
}

void Cache::Clear()
{
  m_notReadyHotels.clear();
  m_unavailableHotels.clear();
  m_availableHotels.clear();
}

template <typename Container>
void Cache::ReserveAdditional(Container & container, size_t additionalCount)
{
  if (m_maxCount != 0 && (container.size() + additionalCount) > m_maxCount)
    container.clear();
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
