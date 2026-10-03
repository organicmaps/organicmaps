#pragma once

#include "storage/storage_defines.hpp"

#include "base/deferred_task.hpp"

#include <chrono>
#include <cstddef>
#include <utility>

class DownloadingPolicy
{
public:
  using TProcessFunc = std::function<void(storage::CountriesSet const &)>;
  virtual ~DownloadingPolicy() = default;
  virtual bool IsDownloadingAllowed() { return true; }
  virtual void ScheduleRetry(storage::CountriesSet const &, TProcessFunc const &) {}
  // The terrain failures retry independently: one deferred slot per family, so a map
  // arming cannot swallow a pending terrain retry (and vice versa).
  virtual void ScheduleTerrainRetry(storage::CountriesSet const &, TProcessFunc const &,
                                    bool /* hasNonRetryableFailures */ = false)
  {}
};

class StorageDownloadingPolicy : public DownloadingPolicy
{
  bool m_cellularDownloadEnabled = false;
  bool m_downloadRetryFailed = false;
  bool m_terrainDownloadRetryFailed = false;
  static size_t constexpr kAutoRetryCounterMax = 3;
  size_t m_autoRetryCounter = kAutoRetryCounterMax;
  base::DeferredTask m_autoRetryWorker;
  size_t m_terrainRetryCounter = kAutoRetryCounterMax;
  base::DeferredTask m_terrainRetryWorker;

  std::chrono::time_point<std::chrono::steady_clock> m_disableCellularTime;

public:
  explicit StorageDownloadingPolicy(base::DeferredTask::Duration retryInterval = std::chrono::seconds(20))
    : m_autoRetryWorker(retryInterval)
    , m_terrainRetryWorker(retryInterval)
  {}
  void EnableCellularDownload(bool enabled);
  bool IsCellularDownloadEnabled();

  bool IsAutoRetryDownloadFailed() const
  {
    return m_downloadRetryFailed || m_autoRetryCounter == 0 || m_terrainDownloadRetryFailed ||
           m_terrainRetryCounter == 0;
  }

  // DownloadingPolicy overrides:
  bool IsDownloadingAllowed() override;
  void ScheduleRetry(storage::CountriesSet const & failedCountries, TProcessFunc const & func) override;
  void ScheduleTerrainRetry(storage::CountriesSet const & failedRegions, TProcessFunc const & func,
                            bool hasNonRetryableFailures = false) override;
};
