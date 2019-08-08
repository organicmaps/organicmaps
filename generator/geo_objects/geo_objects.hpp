#pragma once

#include "generator/key_value_storage.hpp"

#include "generator/regions/region_info_getter.hpp"

#include "generator/geo_objects/geo_objects_maintainer.hpp"

#include "geometry/meter.hpp"
#include "geometry/point2d.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include "platform/platform.hpp"

#include <string>

namespace generator
{
namespace geo_objects
{
class RegionInfoGetterProxy
{
public:
  using RegionInfoGetter = std::function<boost::optional<KeyValue>(m2::PointD const & pathPoint)>;
  RegionInfoGetterProxy(std::string const & pathInRegionsIndex,
                        std::string const & pathInRegionsKv)
  {
    m_regionInfoGetter = regions::RegionInfoGetter(pathInRegionsIndex, pathInRegionsKv);
    LOG(LINFO, ("Size of regions key-value storage:", m_regionInfoGetter->GetStorage().Size()));
  }

  explicit RegionInfoGetterProxy(RegionInfoGetter && regionInfoGetter)
      : m_externalInfoGetter(std::move(regionInfoGetter))
  {
    LOG(LINFO, ("External regions info provided"));
  }

  boost::optional<KeyValue> FindDeepest(m2::PointD const & point) const
  {
    return m_regionInfoGetter ? m_regionInfoGetter->FindDeepest(point)
                              : m_externalInfoGetter->operator()(point);
  }
private:
  boost::optional<regions::RegionInfoGetter> m_regionInfoGetter;
  boost::optional<RegionInfoGetter> m_externalInfoGetter;
};


using IndexReader = ReaderPtr<Reader>;

boost::optional<indexer::GeoObjectsIndex<IndexReader>> MakeTempGeoObjectsIndex(
    std::string const & pathToGeoObjectsTmpMwm);

bool JsonHasBuilding(JsonValue const & json);

void AddBuildingsAndThingsWithHousesThenEnrichAllWithRegionAddresses(
    KeyValueStorage & geoObjectsKv, RegionInfoGetterProxy const & regionInfoGetter,
    std::string const & pathInGeoObjectsTmpMwm, bool verbose, size_t threadsCount);

struct NullBuildingsInfo
{
  std::unordered_map<base::GeoObjectId, base::GeoObjectId> m_addressPoints2Buildings;
  // Quite possible to have many points for one building. We want to use
  // their addresses for POIs according to buildings and have no idea how to distinguish between
  // them, so take one random
  std::unordered_map<base::GeoObjectId, base::GeoObjectId> m_Buildings2AddressPoint;
};

NullBuildingsInfo EnrichPointsWithOuterBuildingGeometry(
    GeoObjectMaintainer const & geoObjectMaintainer, std::string const & pathInGeoObjectsTmpMwm,
    size_t threadsCount);


void AddPoisEnrichedWithHouseAddresses(KeyValueStorage & geoObjectsKv,
                                       GeoObjectMaintainer const & geoObjectMaintainer,
                                       NullBuildingsInfo const & buildingsInfo,
                                       std::string const & pathInGeoObjectsTmpMwm,
                                       std::ostream & streamPoiIdsToAddToLocalityIndex,
                                       bool verbose, size_t threadsCount);


void FilterAddresslessThanGaveTheirGeometryToInnerPoints(std::string const & pathInGeoObjectsTmpMwm,
                                                         NullBuildingsInfo const & buildingsInfo,
                                                         size_t threadsCount);

}  // namespace geo_objects
}  // namespace generator
