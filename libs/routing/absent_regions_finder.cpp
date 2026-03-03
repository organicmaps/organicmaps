#include "routing/absent_regions_finder.hpp"
#include "routing/regions_router.hpp"

#include "geometry/mercator.hpp"

namespace routing
{
AbsentRegionsFinder::AbsentRegionsFinder(CountryFileGetterFn countryFileGetter, LocalFileCheckerFn localFileChecker,
                                         std::shared_ptr<NumMwmIds> numMwmIds, DataSource & dataSource)
  : m_countryFileGetterFn(std::move(countryFileGetter))
  , m_localFileCheckerFn(std::move(localFileChecker))
  , m_numMwmIds(std::move(numMwmIds))
  , m_dataSource(dataSource)
{
  CHECK(m_countryFileGetterFn, ());
  CHECK(m_localFileCheckerFn, ());
}

RouterResultCode AbsentRegionsFinder::GenerateAbsentRegions(Checkpoints const & checkpoints,
                                                            RouterDelegate const & delegate)
{
  m_absentRegions.clear();
  if (m_routerThread)
  {
    m_routerThread->Cancel();
    m_routerThread.reset();
  }

  std::string currCountry, prevCountry;
  bool differentRegions = false;

  for (auto const & checkpoint : checkpoints.GetPoints())
  {
    currCountry = m_countryFileGetterFn(checkpoint);
    if (currCountry.empty())
    {
      /// @todo We can move checkpoint to the nearest Region's polygon here.
      LOG(LWARNING, ("For point", mercator::ToLatLon(checkpoint),
                     "CountryInfoGetter returns empty. It happens when checkpoint is put at gaps between MWMs."));
      return RouterResultCode::InternalError;
    }

    if (!m_localFileCheckerFn(currCountry))
      m_absentRegions.insert(currCountry);

    if (!prevCountry.empty() && prevCountry != currCountry)
      differentRegions = true;

    prevCountry.swap(currCountry);
  }

  if (differentRegions)
  {
    auto router =
        std::make_unique<RegionsRouter>(m_countryFileGetterFn, m_numMwmIds, m_dataSource, delegate, checkpoints);

    // iOS can't reuse threads. So we need to recreate the thread.
    m_routerThread = std::make_unique<threads::Thread>();
    m_routerThread->Create(std::move(router));
  }

  return (!m_absentRegions.empty() ? RouterResultCode::NeedMoreMaps : RouterResultCode::NoError);
}

AbsentRegionsFinder::RegionsSetT AbsentRegionsFinder::GetAbsentRegions()
{
  auto regions = GetAllRegions();
  for (auto i = regions.begin(); i != regions.end();)
    if (m_localFileCheckerFn(*i))
      i = regions.erase(i);
    else
      ++i;

  for (auto & country : m_absentRegions)
    regions.insert(std::move(country));
  m_absentRegions.clear();

  return regions;
}

AbsentRegionsFinder::RegionsSetT AbsentRegionsFinder::GetAllRegions()
{
  if (!m_routerThread)
    return {};

  m_routerThread->Join();

  RegionsSetT result = m_routerThread->GetRoutineAs<RegionsRouter>()->GetMwmNames();

  m_routerThread.reset();

  return result;
}

}  // namespace routing
