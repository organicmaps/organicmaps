#pragma once

#include "partners_api/booking_api.hpp"

#include "base/macros.hpp"

#include <chrono>
#include <map>
#include <optional>
#include <utility>

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

  struct Info
  {
    Info() = default;
    explicit Info(HotelStatus status) : m_status(status) {}
    Info(HotelStatus status, Extras const & extras) : m_status(status), m_extras(extras) {}

    HotelStatus m_status = HotelStatus::Absent;
    std::optional<Extras> m_extras;
  };

  Cache() = default;
  Cache(size_t maxCount, size_t expiryPeriodSeconds);

  Info Get(std::string const & hotelId);
  // Removes items from available and unavailable
  // in case of (current items count + |count|) >= |m_maxCount|.
  void ReserveAdditional(size_t count);

  void InsertNotReady(std::string const & hotelId);
  void InsertUnavailable(std::string const & hotelId);
  void InsertAvailable(std::string const & hotelId, Extras && extras);

  void RemoveOutdated();
  void Clear();

private:
  struct Item
  {
    Item() = default;
    explicit Item(Extras && extras) : m_extras(std::move(extras)) {}

    Clock::time_point m_timestamp = Clock::now();
    Extras m_extras;
  };

  using HotelWithTimestampMap = std::map<std::string, Clock::time_point>;
  using HotelWithExtrasMap = std::map<std::string, Item>;
  // Removes items in case size >= |m_maxCount|.
  template <typename Container>
  void ReserveAdditional(Container & container, size_t additionalCount);

  HotelWithTimestampMap m_notReadyHotels;
  HotelWithTimestampMap m_unavailableHotels;
  HotelWithExtrasMap m_availableHotels;
  // Max count of |m_availableHotels| or |m_unavailableHotels| container.
  // Count is unlimited when |m_maxCount| is equal to zero.
  size_t const m_maxCount = 3000;
  // Do not use aging when |m_expiryPeriodSeconds| is equal to zero.
  size_t const m_expiryPeriodSeconds = 300;
};

std::string DebugPrint(Cache::HotelStatus status);
}  // namespace availability
}  // namespace filter
}  // namespace booking
