#pragma once

#include "storage/index.hpp"

#include "base/deferred_task.hpp"

#include "std/chrono.hpp"
#include "std/function.hpp"

class DownloadingPolicy
{
public:
  using TProcessFunc = function<void(storage::TCountriesSet const &)>;
  virtual ~DownloadingPolicy() = default;
  virtual bool IsDownloadingAllowed() { return true; }
  virtual void ScheduleRetry(storage::TCountriesSet const &, TProcessFunc const &) {}
};

class StorageDownloadingPolicy : public DownloadingPolicy
{
  bool m_cellularDownloadEnabled = false;
  bool m_downloadRetryFailed = false;
  static size_t constexpr kAutoRetryCounterMax = 3;
  size_t m_autoRetryCounter = kAutoRetryCounterMax;
  my::DeferredTask m_autoRetryWorker;

  time_point<steady_clock> m_disableCellularTime;

public:
  StorageDownloadingPolicy() : m_autoRetryWorker(seconds(20)) {}
  void EnableCellularDownload(bool enabled);
  bool IsCellularDownloadEnabled();

  inline bool IsAutoRetryDownloadFailed() const
  {
    return m_downloadRetryFailed || m_autoRetryCounter == 0;
  }

  // DownloadingPolicy overrides:
  bool IsDownloadingAllowed() override;
  void ScheduleRetry(storage::TCountriesSet const & failedCountries,
                     TProcessFunc const & func) override;
};
