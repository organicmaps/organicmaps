#pragma once

#include "storage/index.hpp"

#include "base/deferred_task.hpp"

#include "std/chrono.hpp"
#include "std/function.hpp"

class DownloadingPolicy
{
public:
  using TProcessFunc = function<void(storage::TCountriesSet const &)>;
  virtual bool IsDownloadingAllowed() const { return true; }
  virtual void ScheduleRetry(storage::TCountriesSet const &, TProcessFunc const &) {}
};

class StorageDownloadingPolicy : public DownloadingPolicy
{
  bool m_cellularDownloadEnabled = false;
  static size_t constexpr kAutoRetryCounterMax = 3;
  size_t m_autoRetryCounter = kAutoRetryCounterMax;
  my::DeferredTask m_autoRetryWorker;
  
public:
  StorageDownloadingPolicy() : m_autoRetryWorker(seconds(20)) {}
  
  inline void EnableCellularDownload(bool value) { m_cellularDownloadEnabled = value; }
  inline bool IsCellularDownloadEnabled() const { return m_cellularDownloadEnabled; }
  inline bool IsAutoRetryDownloadFailed() const { return m_autoRetryCounter == 0; }

  bool IsDownloadingAllowed() const override;
  void ScheduleRetry(storage::TCountriesSet const & failedCountries, TProcessFunc const & func) override;
};
