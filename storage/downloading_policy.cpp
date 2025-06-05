#include "storage/downloading_policy.hpp"

#include "platform/platform.hpp"

using namespace std::chrono;

void StorageDownloadingPolicy::EnableCellularDownload(bool enabled)
{
  m_cellularDownloadEnabled = enabled;
  m_disableCellularTime = steady_clock::now() + hours(1);
}

bool StorageDownloadingPolicy::IsCellularDownloadEnabled()
{
  if (m_cellularDownloadEnabled && steady_clock::now() > m_disableCellularTime)
    m_cellularDownloadEnabled = false;

  return m_cellularDownloadEnabled;
}

bool StorageDownloadingPolicy::IsDownloadingAllowed()
{
  return !(GetPlatform().ConnectionStatus() == Platform::EConnectionType::CONNECTION_WWAN &&
           !IsCellularDownloadEnabled());
}

void StorageDownloadingPolicy::ScheduleRetry(storage::CountriesSet const & failedCountries, TProcessFunc const & func)
{
  if (IsDownloadingAllowed() && !failedCountries.empty() && m_autoRetryCounter > 0)
  {
    m_downloadRetryFailed = false;
    auto action = [this, func, failedCountries]
    {
      --m_autoRetryCounter;
      func(failedCountries);
    };
    m_autoRetryWorker.RestartWith([action] { GetPlatform().RunTask(Platform::Thread::Gui, action); });
  }
  else
  {
    if (!failedCountries.empty())
      m_downloadRetryFailed = true;
    m_autoRetryCounter = kAutoRetryCounterMax;
  }
}
