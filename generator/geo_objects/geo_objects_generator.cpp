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

GeoObjectsGenerator::GeoObjectsGenerator(
    GeoObjectMaintainer::RegionInfoGetter && regionInfoGetter,
    GeoObjectMaintainer::RegionIdGetter && regionIdGetter, std::string pathInGeoObjectsTmpMwm,
    std::string pathOutIdsWithoutAddress, std::string pathOutGeoObjectsKv, bool verbose,
    size_t threadsCount)
  : m_pathInGeoObjectsTmpMwm(std::move(pathInGeoObjectsTmpMwm))
  , m_pathOutPoiIdsToAddToLocalityIndex(std::move(pathOutIdsWithoutAddress))
  , m_pathOutGeoObjectsKv(std::move(pathOutGeoObjectsKv))
  , m_verbose(verbose)
  , m_threadsCount(threadsCount)
  , m_geoObjectMaintainer{m_pathOutGeoObjectsKv, std::move(regionInfoGetter), std::move(regionIdGetter)}
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
      m_geoObjectMaintainer, m_pathInGeoObjectsTmpMwm, m_verbose, m_threadsCount);

  LOG(LINFO, ("Geo objects with addresses were built."));

  auto geoObjectIndex = geoObjectIndexFuture.get();

  LOG(LINFO, ("Index was built."));

  if (!geoObjectIndex)
    return false;

  m_geoObjectMaintainer.SetIndex(std::move(*geoObjectIndex));

  LOG(LINFO, ("Enrich address points with outer null building geometry."));

  NullBuildingsInfo const & buildingInfo = EnrichPointsWithOuterBuildingGeometry(
      m_geoObjectMaintainer, m_pathInGeoObjectsTmpMwm, m_threadsCount);

  std::ofstream streamPoiIdsToAddToLocalityIndex(m_pathOutPoiIdsToAddToLocalityIndex);

  AddPoisEnrichedWithHouseAddresses(m_geoObjectMaintainer, buildingInfo, m_pathInGeoObjectsTmpMwm,
                                    streamPoiIdsToAddToLocalityIndex, m_verbose, m_threadsCount);

  FilterAddresslessThanGaveTheirGeometryToInnerPoints(m_pathInGeoObjectsTmpMwm, buildingInfo,
                                                      m_threadsCount);

  LOG(LINFO, ("Addressless buildings with geometry we used for inner points were filtered"));

  LOG(LINFO, ("Geo objects without addresses were built."));
  LOG(LINFO, ("Geo objects key-value storage saved to", m_pathOutGeoObjectsKv));
  LOG(LINFO, ("Ids of POIs without addresses saved to", m_pathOutPoiIdsToAddToLocalityIndex));
  return true;
}

bool GenerateGeoObjects(std::string const & regionsIndex, std::string const & regionsKeyValue,
                        std::string const & geoObjectsFeatures,
                        std::string const & nodesListToIndex, std::string const & geoObjectKeyValue,
                        bool verbose, size_t threadsCount)

{
  auto regionInfoGetter = regions::RegionInfoGetter(regionsIndex, regionsKeyValue);
  LOG(LINFO, ("Size of regions key-value storage:", regionInfoGetter.GetStorage().Size()));

  auto findDeepest = [&regionInfoGetter](auto && point) {
    return regionInfoGetter.FindDeepest(point);
  };
  auto keyValueFind = [&regionInfoGetter](auto && id) {
    return regionInfoGetter.GetStorage().Find(id.GetEncodedId());
  };

  geo_objects::GeoObjectsGenerator geoObjectsGenerator{std::move(findDeepest),
                                                       std::move(keyValueFind),
                                                       geoObjectsFeatures,
                                                       nodesListToIndex,
                                                       geoObjectKeyValue,
                                                       verbose,
                                                       threadsCount};

  return geoObjectsGenerator.GenerateGeoObjects();
}
}  // namespace geo_objects
}  // namespace generator
