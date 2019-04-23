#include "generator/streets/streets.hpp"

#include "generator/regions/region_info_getter.hpp"
#include "generator/streets/streets_builder.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include <fstream>

namespace generator
{
namespace streets
{
void GenerateStreets(std::string const & pathInRegionsIndex, std::string const & pathInRegionsKv,
                     std::string const & pathInStreetsTmpMwm,
                     std::string const & pathInGeoObjectsTmpMwm,
                     std::string const & pathOutStreetsKv, bool verbose)
{
  LOG(LINFO, ("Start generating streets..."));
  auto timer = base::Timer();
  SCOPE_GUARD(finishGeneratingStreets, [&timer]() {
    LOG(LINFO, ("Finish generating streets.", timer.ElapsedSeconds(), "seconds."));
  });

  regions::RegionInfoGetter regionInfoGetter{pathInRegionsIndex, pathInRegionsKv};
  LOG(LINFO, ("Size of regions key-value storage:", regionInfoGetter.GetStorage().Size()));

  StreetsBuilder streetsBuilder{regionInfoGetter};

  streetsBuilder.AssembleStreets(pathInStreetsTmpMwm);
  LOG(LINFO, ("Streets were built."));

  streetsBuilder.AssembleBindings(pathInGeoObjectsTmpMwm);
  LOG(LINFO, ("Binding's streets were built."));

  std::ofstream streamStreetsKv(pathOutStreetsKv);
  streetsBuilder.SaveStreetsKv(streamStreetsKv);
  LOG(LINFO, ("Streets key-value storage saved to", pathOutStreetsKv));
}
}  // namespace streets
}  // namespace generator
