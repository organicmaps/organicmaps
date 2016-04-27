#include "storage/downloading_policy.hpp"

#include "platform/platform.hpp"

bool StorageDownloadingPolicy::IsDownloadingAllowed() const
{
  return !(GetPlatform().ConnectionStatus() == Platform::EConnectionType::CONNECTION_WWAN &&
           !IsCellularDownloadEnabled());
}

void StorageDownloadingPolicy::ScheduleRetry(storage::TCountriesSet const & failedCountries,
                                             TProcessFunc const & func)
{
  if (IsDownloadingAllowed() && !failedCountries.empty() && m_autoRetryCounter > 0)
  {
    auto action = [this, func, failedCountries]
    {
      --m_autoRetryCounter;
      func(failedCountries);
    };
    m_autoRetryWorker.RestartWith([action]{ Platform().RunOnGuiThread(action); });
  }
  else
  {
    m_autoRetryCounter = kAutoRetryCounterMax;
  }
}
