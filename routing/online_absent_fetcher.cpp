#include "routing/online_absent_fetcher.hpp"

#include "routing/online_cross_fetcher.hpp"

#include "platform/platform.hpp"
#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"

#include "base/stl_helpers.hpp"

#include "std/vector.hpp"

#include "private.h"

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
  if (GetPlatform().ConnectionStatus() == Platform::EConnectionType::CONNECTION_NONE)
    return;

  // Single mwm case.
  if (AllPointsInSameMwm(checkpoints))
    return;

  unique_ptr<OnlineCrossFetcher> fetcher =
      make_unique<OnlineCrossFetcher>(m_countryFileFn, OSRM_ONLINE_SERVER_URL, checkpoints);
  // iOS can't reuse threads. So we need to recreate the thread.
  m_fetcherThread.reset(new threads::Thread());
  m_fetcherThread->Create(move(fetcher));
}

void OnlineAbsentCountriesFetcher::GetAbsentCountries(vector<string> & countries)
{
  countries.clear();
  // Check whether a request was scheduled to be run on the thread.
  if (!m_fetcherThread)
    return;

  m_fetcherThread->Join();
  for (auto const & point : m_fetcherThread->GetRoutineAs<OnlineCrossFetcher>()->GetMwmPoints())
  {
    string const name = m_countryFileFn(point);
    ASSERT(!name.empty(), ());
    if (name.empty() || m_countryLocalFileFn(name))
      continue;

    countries.emplace_back(move(name));
  }
  m_fetcherThread.reset();

  my::SortUnique(countries);
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
