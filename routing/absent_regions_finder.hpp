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

// Encapsulates generation of mwm names of absent regions needed for building the route between
// |checkpoints|. For this purpose the new thread is used.
class AbsentRegionsFinder
{
public:
  AbsentRegionsFinder(CountryFileGetterFn const & countryFileGetter, LocalFileCheckerFn const & localFileChecker,
                      std::shared_ptr<NumMwmIds> numMwmIds, DataSource & dataSource);

  // Creates new thread |m_routerThread| and starts routing in it.
  void GenerateAbsentRegions(Checkpoints const & checkpoints, RouterDelegate const & delegate);
  // Waits for the routing thread |m_routerThread| to finish and returns results from it.
  void GetAllRegions(std::set<std::string> & countries);
  // Waits for the results from GetAllRegions() and returns only regions absent on the device.
  void GetAbsentRegions(std::set<std::string> & absentCountries);

private:
  bool AreCheckpointsInSameMwm(Checkpoints const & checkpoints) const;

  CountryFileGetterFn const m_countryFileGetterFn;
  LocalFileCheckerFn const m_localFileCheckerFn;

  std::shared_ptr<NumMwmIds> m_numMwmIds;
  DataSource & m_dataSource;

  std::unique_ptr<threads::Thread> m_routerThread;
};
}  // namespace routing
