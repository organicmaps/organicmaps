#include "online_absent_fetcher.hpp"

#include "defines.hpp"
#include "online_cross_fetcher.hpp"

namespace routing
{
void OnlineAbsentFetcher::GenerateRequest(const m2::PointD & startPoint,
                                          const m2::PointD & finalPoint)
{
  // single mwm case
  if (m_countryFunction(startPoint) == m_countryFunction(finalPoint))
    return;
  unique_ptr<OnlineCrossFetcher> fetcher =
      make_unique<OnlineCrossFetcher>(OSRM_ONLINE_SERVER_URL, startPoint, finalPoint);
  m_fetcherThread.Create(move(fetcher));
}

void OnlineAbsentFetcher::GetAbsentCountries(vector<string> & countries)
{
  // Have task check
  if (!m_fetcherThread.GetRoutine())
    return;
  m_fetcherThread.Join();
  for (auto point : static_cast<OnlineCrossFetcher *>(m_fetcherThread.GetRoutine())->GetMwmPoints())
  {
    string name = m_countryFunction(point);
    //TODO (ldragunov) rewrite when storage GetLocalCountryFile will be present.
    Platform & pl = GetPlatform();
    string const mwmName = name + DATA_FILE_EXTENSION;
    string const fPath = pl.WritablePathForFile(mwmName + ROUTING_FILE_EXTENSION);
    if (!pl.IsFileExistsByFullPath(fPath))
    {
      LOG(LINFO, ("Online recomends to download: ", name));
      countries.emplace_back(move(name));
    }
  }
}
}  // namespace routing
