#pragma once

#include "base/macros.hpp"

#include <chrono>
#include <map>

namespace booking
{
namespace filter
{
namespace availability
{
class Cache
{
public:
  enum class HotelStatus
  {
    // The hotel is absent in cache.
    Absent,
    // Information about the hotel was requested, but request is not ready yet.
    NotReady,
    // The hotel is unavailable for booking.
    Unavailable,
    // The hotel is available for booking.
    Available,
  };

  using Clock = std::chrono::steady_clock;

  struct Item
  {
    Item() = default;

    explicit Item(HotelStatus s) : m_status(s) {}

    Clock::time_point m_timestamp = Clock::now();
    HotelStatus m_status = HotelStatus::NotReady;
  };

  Cache() = default;
  Cache(size_t maxCount, size_t expiryPeriodSeconds);

  HotelStatus Get(std::string const & hotelId);
  void Reserve(std::string const & hotelId);
  void Insert(std::string const & hotelId, HotelStatus const s);

  void RemoveOutdated();
  void Clear();

private:
  using HotelsMap = std::map<std::string, Item>;
  // In case when size >= |m_maxCount| removes items except those who have the status
  // HotelStatus::NotReady.
  void RemoveExtra();
  bool IsExpired(Clock::time_point const & timestamp) const;
  HotelStatus Get(HotelsMap & src, std::string const & hotelId);
  void RemoveOutdated(HotelsMap & src);

  HotelsMap m_hotelToStatus;
  HotelsMap m_notReadyHotels;
  // Max count of |m_hotelToStatus| container.
  // Count is unlimited when |m_maxCount| is equal to zero.
  size_t const m_maxCount = 1000;
  // Do not use aging when |m_expiryPeriodSeconds| is equal to zero.
  size_t const m_expiryPeriodSeconds = 60;
};

std::string DebugPrint(Cache::HotelStatus status);
}  // namespace availability
}  // namespace filter
}  // namespace booking
