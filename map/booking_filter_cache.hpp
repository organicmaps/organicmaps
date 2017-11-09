#pragma once

#include "base/worker_thread.hpp"

#include <map>
#include <mutex>

namespace booking
{
namespace filter
{
namespace availability
{
// *NOTE* This class IS thread-safe.
class Cache
{
public:
  enum class HotelStatus
  {
    Absent,
    NotReady,
    UnAvailable,
    Available,
  };

  struct Item
  {
    Item() = default;

    explicit Item(HotelStatus const s) : m_status(s) {}

    base::WorkerThread::TimePoint m_timestamp = base::WorkerThread::Now();
    HotelStatus m_status = HotelStatus::NotReady;
  };

  Cache() = default;
  Cache(size_t maxCount, size_t expiryPeriodSeconds);

  HotelStatus Get(std::string const & hotelId) const;
  void Reserve(std::string const & hotelId);
  void Insert(std::string const & hotelId, HotelStatus const s);

  DISALLOW_COPY_AND_MOVE(Cache);

private:
  void RemoveOutdated();
  void RemoveExtra();

  std::map<std::string, Item> m_hotelToResult;
  mutable std::mutex m_mutex;
  int m_agingInProgress = false;
  base::WorkerThread m_agingThread;
  // Count is unlimited when |m_maxCount| is equal to zero.
  size_t const m_maxCount = 1000;
  // Aging process is disabled when |m_expiryPeriodSeconds| is equal to zero.
  size_t const m_expiryPeriodSeconds = 60;
};

std::string DebugPrint(Cache::HotelStatus status);
}  // namespace availability
}  // namespace filter
}  // namespace booking
