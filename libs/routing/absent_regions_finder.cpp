#include "routing/absent_regions_finder.hpp"
#include "routing/regions_router.hpp"

namespace routing
{
AbsentRegionsFinder::AbsentRegionsFinder(CountryFileGetterFn const & countryFileGetter,
                                         LocalFileCheckerFn const & localFileChecker,
                                         std::shared_ptr<NumMwmIds> numMwmIds, DataSource & dataSource)
  : m_countryFileGetterFn(countryFileGetter)
  , m_localFileCheckerFn(localFileChecker)
  , m_numMwmIds(std::move(numMwmIds))
  , m_dataSource(dataSource)
{
  CHECK(m_countryFileGetterFn, ());
  CHECK(m_localFileCheckerFn, ());
}

void AbsentRegionsFinder::GenerateAbsentRegions(Checkpoints const & checkpoints, RouterDelegate const & delegate)
{
  if (m_routerThread)
  {
    m_routerThread->Cancel();
    m_routerThread.reset();
  }

  if (AreCheckpointsInSameMwm(checkpoints))
    return;

  std::unique_ptr<RegionsRouter> router =
      std::make_unique<RegionsRouter>(m_countryFileGetterFn, m_numMwmIds, m_dataSource, delegate, checkpoints);

  // iOS can't reuse threads. So we need to recreate the thread.
  m_routerThread = std::make_unique<threads::Thread>();
  m_routerThread->Create(std::move(router));
}

void AbsentRegionsFinder::GetAbsentRegions(std::set<std::string> & regions)
{
  regions.clear();
  GetAllRegions(regions);

  for (auto i = regions.begin(); i != regions.end();)
    if (m_localFileCheckerFn(*i))
      i = regions.erase(i);
    else
      ++i;
}

void AbsentRegionsFinder::GetAllRegions(std::set<std::string> & countries)
{
  countries.clear();

  if (!m_routerThread)
    return;

  m_routerThread->Join();

  for (auto const & mwmName : m_routerThread->GetRoutineAs<RegionsRouter>()->GetMwmNames())
    if (!mwmName.empty())
      countries.emplace(mwmName);

  m_routerThread.reset();
}

bool AbsentRegionsFinder::AreCheckpointsInSameMwm(Checkpoints const & checkpoints) const
{
  for (size_t i = 0; i < checkpoints.GetNumSubroutes(); ++i)
    if (m_countryFileGetterFn(checkpoints.GetPoint(i)) != m_countryFileGetterFn(checkpoints.GetPoint(i + 1)))
      return false;

  return true;
}
}  // namespace routing
