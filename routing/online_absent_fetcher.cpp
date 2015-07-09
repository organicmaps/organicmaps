#include "online_absent_fetcher.hpp"

#include "defines.hpp"
#include "online_cross_fetcher.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"

#include "std/vector.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

namespace routing
{
void OnlineAbsentCountriesFetcher::GenerateRequest(const m2::PointD & startPoint,
                                                   const m2::PointD & finalPoint)
{
  // Single mwm case.
  if (m_countryFileFn(startPoint) == m_countryFileFn(finalPoint))
    return;
  unique_ptr<OnlineCrossFetcher> fetcher =
      make_unique<OnlineCrossFetcher>(OSRM_ONLINE_SERVER_URL, MercatorBounds::ToLatLon(startPoint),
                                      MercatorBounds::ToLatLon(finalPoint));
  // iOS can't reuse threads. So we need to recreate the thread.
  m_fetcherThread.reset(new threads::Thread());
  m_fetcherThread->Create(move(fetcher));
}

void OnlineAbsentCountriesFetcher::GetAbsentCountries(vector<string> & countries)
{
  // Check whether a request was scheduled to be run on the thread.
  if (!m_fetcherThread)
    return;
  m_fetcherThread->Join();
  for (auto const & point : m_fetcherThread->GetRoutineAs<OnlineCrossFetcher>()->GetMwmPoints())
  {
    string name = m_countryFileFn(point);
    auto localFile = m_countryLocalFileFn(name);
    if (localFile && HasOptions(localFile->GetFiles(), TMapOptions::EMapWithCarRouting))
      continue;

    LOG(LINFO, ("Online absent countries fetcher recomends to download: ", name));
    countries.emplace_back(move(name));
  }
  m_fetcherThread.reset();
}
}  // namespace routing
