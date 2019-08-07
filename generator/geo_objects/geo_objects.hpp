#pragma once

#include "generator/key_value_storage.hpp"
#include "generator/regions/region_info_getter.hpp"

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
using IndexReader = ReaderPtr<Reader>;

boost::optional<indexer::GeoObjectsIndex<IndexReader>> MakeTempGeoObjectsIndex(
    std::string const & pathToGeoObjectsTmpMwm);

bool JsonHasBuilding(JsonValue const & json);

class GeoObjectsGenerator
{
public:
  using RegionInfoGetter = std::function<boost::optional<KeyValue>(m2::PointD const & pathPoint)>;

  class RegionInfoGetterProxy
  {
  public:
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

  GeoObjectsGenerator(std::string pathInRegionsIndex, std::string pathInRegionsKv,
                      std::string pathInGeoObjectsTmpMwm, std::string pathOutIdsWithoutAddress,
                      std::string pathOutGeoObjectsKv, bool verbose, size_t threadsCount);

  GeoObjectsGenerator(RegionInfoGetter && regionInfoGetter, std::string pathInGeoObjectsTmpMwm,
                      std::string pathOutIdsWithoutAddress, std::string pathOutGeoObjectsKv,
                      bool verbose, size_t threadsCount);

  // This function generates key-value pairs for geo objects.
  // First, we try to generate key-value pairs only for houses, since we cannot say anything about
  // poi. In this step, we need key-value pairs for the regions and the index for the regions. Then
  // we build an index for houses. And then we finish building key-value pairs for poi using this
  // index for houses.
  bool GenerateGeoObjects();

  KeyValueStorage const & GetKeyValueStorage() const
  {
    return m_geoObjectsKv;
  }

private:
  bool GenerateGeoObjectsPrivate();
  static KeyValueStorage InitGeoObjectsKv(std::string const & pathOutGeoObjectsKv)
  {
    Platform().RemoveFileIfExists(pathOutGeoObjectsKv);
    return KeyValueStorage(pathOutGeoObjectsKv, 0 /* cacheValuesCountLimit */);
  }

  std::string m_pathInGeoObjectsTmpMwm;
  std::string m_pathOutIdsWithoutAddress;
  std::string m_pathOutGeoObjectsKv;

  bool m_verbose = false;
  size_t m_threadsCount = 1;

  KeyValueStorage m_geoObjectsKv;
  RegionInfoGetterProxy m_regionInfoGetter;
};
}  // namespace geo_objects
}  // namespace generator
