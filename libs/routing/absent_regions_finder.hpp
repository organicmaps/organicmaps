#pragma once

#include "routing/regions_decl.hpp"
#include "routing/router_delegate.hpp"

#include "base/thread.hpp"

#include <functional>
#include <memory>
#include <set>
#include <string>

namespace routing
{
using LocalFileCheckerFn = std::function<bool(std::string const &)>;

// Finds missing checkpoint maps. Road routing additionally searches intermediate
// regions on a worker thread; the checkpoint-only constructor does not.
class AbsentRegionsFinder
{
public:
  AbsentRegionsFinder(CountryFileGetterFn countryFileGetter, LocalFileCheckerFn localFileChecker,
                      std::shared_ptr<NumMwmIds> numMwmIds, DataSource & dataSource);
  // Check checkpoint maps only, without inferring a route through the road network.
  AbsentRegionsFinder(CountryFileGetterFn countryFileGetter, LocalFileCheckerFn localFileChecker);
  ~AbsentRegionsFinder();

  // Checks checkpoints and, when configured, starts a road-region search.
  RouterResultCode GenerateAbsentRegions(Checkpoints const & checkpoints, RouterDelegate const & delegate);

  using RegionsSetT = std::set<std::string>;

  // Waits for the routing thread |m_routerThread| to finish and returns results from it.
  RegionsSetT GetAllRegions();
  // Waits for the results from GetAllRegions() and returns only regions absent on the device.
  RegionsSetT GetAbsentRegions();

private:
  CountryFileGetterFn const m_countryFileGetterFn;
  LocalFileCheckerFn const m_localFileCheckerFn;

  RegionsSetT m_absentRegions;

  std::shared_ptr<NumMwmIds> m_numMwmIds;
  DataSource * m_dataSource = nullptr;

  std::unique_ptr<threads::Thread> m_routerThread;
};
}  // namespace routing
