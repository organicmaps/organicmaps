#include "generator/key_value_storage.hpp"

#include "generator/regions/region_info_getter.hpp"

#include "generator/geo_objects/geo_objects.hpp"

#include "geometry/meter.hpp"
#include "geometry/point2d.hpp"

#include "platform/platform.hpp"

#include <string>

namespace generator
{
namespace geo_objects
{
class GeoObjectsGenerator
{
public:
  GeoObjectsGenerator(std::string pathInRegionsIndex, std::string pathInRegionsKv,
                      std::string pathInGeoObjectsTmpMwm, std::string pathOutIdsWithoutAddress,
                      std::string pathOutGeoObjectsKv, bool verbose, size_t threadsCount);

  GeoObjectsGenerator(RegionInfoGetterProxy::RegionInfoGetter && regionInfoGetter,
                      std::string pathInGeoObjectsTmpMwm, std::string pathOutIdsWithoutAddress,
                      std::string pathOutGeoObjectsKv, bool verbose, size_t threadsCount);

  // This function generates key-value pairs for geo objects.
  // First, we try to generate key-value pairs only for houses, since we cannot say anything about
  // poi. In this step, we need key-value pairs for the regions and the index for the regions. Then
  // we build an index for houses. And then we finish building key-value pairs for poi using this
  // index for houses.
  bool GenerateGeoObjects();
  GeoObjectMaintainer& GetMaintainer()
  {
    return m_geoObjectMaintainer;
  }

private:
  bool GenerateGeoObjectsPrivate();

  std::string m_pathInGeoObjectsTmpMwm;
  std::string m_pathOutPoiIdsToAddToLocalityIndex;
  std::string m_pathOutGeoObjectsKv;

  bool m_verbose = false;
  size_t m_threadsCount = 1;
  GeoObjectMaintainer m_geoObjectMaintainer;
};
}  // namespace geo_objects
}  // namespace generator
