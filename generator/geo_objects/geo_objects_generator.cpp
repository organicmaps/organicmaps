#include "generator/geo_objects/geo_objects_generator.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include <future>

namespace
{
template <class Activist>
auto Measure(std::string activity, Activist && activist)
{
  LOG(LINFO, ("Start", activity));
  auto timer = base::Timer();
  SCOPE_GUARD(_, [&]() { LOG(LINFO, ("Finish", activity, timer.ElapsedSeconds(), "seconds.")); });

  return activist();
}
}  // namespace

namespace generator
{
namespace geo_objects
{
GeoObjectsGenerator::GeoObjectsGenerator(std::string pathInRegionsIndex,
                                         std::string pathInRegionsKv,
                                         std::string pathInGeoObjectsTmpMwm,
                                         std::string pathOutIdsWithoutAddress,
                                         std::string pathOutGeoObjectsKv, bool verbose,
                                         size_t threadsCount)
  : m_pathInGeoObjectsTmpMwm(std::move(pathInGeoObjectsTmpMwm))
  , m_pathOutPoiIdsToAddToLocalityIndex(std::move(pathOutIdsWithoutAddress))
  , m_pathOutGeoObjectsKv(std::move(pathOutGeoObjectsKv))
  , m_verbose(verbose)
  , m_threadsCount(threadsCount)
  , m_geoObjectsKv(InitGeoObjectsKv(m_pathOutGeoObjectsKv))
  , m_regionInfoGetter(pathInRegionsIndex, pathInRegionsKv)

{
}

GeoObjectsGenerator::GeoObjectsGenerator(
    RegionInfoGetterProxy::RegionInfoGetter && regionInfoGetter, std::string pathInGeoObjectsTmpMwm,
    std::string pathOutIdsWithoutAddress, std::string pathOutGeoObjectsKv, bool verbose,
    size_t threadsCount)
  : m_pathInGeoObjectsTmpMwm(std::move(pathInGeoObjectsTmpMwm))
  , m_pathOutPoiIdsToAddToLocalityIndex(std::move(pathOutIdsWithoutAddress))
  , m_pathOutGeoObjectsKv(std::move(pathOutGeoObjectsKv))
  , m_verbose(verbose)
  , m_threadsCount(threadsCount)
  , m_geoObjectsKv(InitGeoObjectsKv(m_pathOutGeoObjectsKv))
  , m_regionInfoGetter(std::move(regionInfoGetter))
{
}

bool GeoObjectsGenerator::GenerateGeoObjects()
{
  return Measure("generating geo objects", [&]() { return GenerateGeoObjectsPrivate(); });
}

bool GeoObjectsGenerator::GenerateGeoObjectsPrivate()
{
  auto geoObjectIndexFuture =
      std::async(std::launch::async, MakeTempGeoObjectsIndex, m_pathInGeoObjectsTmpMwm);

  AddBuildingsAndThingsWithHousesThenEnrichAllWithRegionAddresses(
      m_geoObjectsKv, m_regionInfoGetter, m_pathInGeoObjectsTmpMwm, m_verbose, m_threadsCount);

  LOG(LINFO, ("Geo objects with addresses were built."));

  auto geoObjectIndex = geoObjectIndexFuture.get();

  LOG(LINFO, ("Index was built."));

  if (!geoObjectIndex)
    return false;

  GeoObjectMaintainer const geoObjectMaintainer{std::move(*geoObjectIndex), m_geoObjectsKv};

  LOG(LINFO, ("Enrich address points with outer null building geometry."));

  NullBuildingsInfo const & buildingInfo = EnrichPointsWithOuterBuildingGeometry(
      geoObjectMaintainer, m_pathInGeoObjectsTmpMwm, m_threadsCount);

  std::ofstream streamPoiIdsToAddToLocalityIndex(m_pathOutPoiIdsToAddToLocalityIndex);

  AddPoisEnrichedWithHouseAddresses(m_geoObjectsKv, geoObjectMaintainer, buildingInfo,
                                    m_pathInGeoObjectsTmpMwm, streamPoiIdsToAddToLocalityIndex,
                                    m_verbose, m_threadsCount);

  FilterAddresslessThanGaveTheirGeometryToInnerPoints(m_pathInGeoObjectsTmpMwm, buildingInfo,
                                                      m_threadsCount);

  LOG(LINFO, ("Addressless buildings with geometry we used for inner points were filtered"));

  LOG(LINFO, ("Geo objects without addresses were built."));
  LOG(LINFO, ("Geo objects key-value storage saved to", m_pathOutGeoObjectsKv));
  LOG(LINFO, ("Ids of POIs without addresses saved to", m_pathOutPoiIdsToAddToLocalityIndex));
  return true;
}

}  // namespace geo_objects
}  // namespace generator
