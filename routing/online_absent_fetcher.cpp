#include "routing/online_absent_fetcher.hpp"

#include "routing/online_cross_fetcher.hpp"

#include "platform/platform.hpp"
#include "platform/country_file.hpp"

#include "base/stl_helpers.hpp"

#include "private.h"

using namespace std;

using platform::CountryFile;
using platform::LocalCountryFile;

namespace routing
{
OnlineAbsentCountriesFetcher::OnlineAbsentCountriesFetcher(
    TCountryFileFn const & countryFileFn, TCountryLocalFileFn const & countryLocalFileFn)
  : m_countryFileFn(countryFileFn), m_countryLocalFileFn(countryLocalFileFn)
{
  CHECK(m_countryFileFn, ());
  CHECK(m_countryLocalFileFn, ());
}

void OnlineAbsentCountriesFetcher::GenerateRequest(Checkpoints const & checkpoints)
{
  if (Platform::ConnectionStatus() == Platform::EConnectionType::CONNECTION_NONE)
    return;

  if (m_fetcherThread)
  {
    m_fetcherThread->Cancel();
    m_fetcherThread.reset();
  }

  // Single mwm case.
  if (AllPointsInSameMwm(checkpoints))
    return;

  unique_ptr<OnlineCrossFetcher> fetcher =
      make_unique<OnlineCrossFetcher>(m_countryFileFn, OSRM_ONLINE_SERVER_URL, checkpoints);
  // iOS can't reuse threads. So we need to recreate the thread.
  m_fetcherThread = make_unique<threads::Thread>();
  m_fetcherThread->Create(move(fetcher));
}

void OnlineAbsentCountriesFetcher::GetAbsentCountries(set<string> & countries)
{
  countries.clear();
  // Check whether a request was scheduled to be run on the thread.
  if (!m_fetcherThread)
    return;

  m_fetcherThread->Join();
  for (auto const & point : m_fetcherThread->GetRoutineAs<OnlineCrossFetcher>()->GetMwmPoints())
  {
    string name = m_countryFileFn(point);
    ASSERT(!name.empty(), ());
    if (name.empty() || m_countryLocalFileFn(name))
      continue;

    countries.emplace(move(name));
  }

  m_fetcherThread.reset();
}

bool OnlineAbsentCountriesFetcher::AllPointsInSameMwm(Checkpoints const & checkpoints) const
{
  for (size_t i = 0; i < checkpoints.GetNumSubroutes(); ++i)
  {
    if (m_countryFileFn(checkpoints.GetPoint(i)) != m_countryFileFn(checkpoints.GetPoint(i + 1)))
      return false;
  }

  return true;
}
}  // namespace routing
