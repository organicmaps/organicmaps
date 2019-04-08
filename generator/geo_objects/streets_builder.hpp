#pragma once

#include "generator/feature_builder.hpp"
#include "generator/geo_objects/key_value_storage.hpp"
#include "generator/geo_objects/region_info_getter.hpp"
#include "generator/osm_element.hpp"

#include "coding/reader.hpp"

#include "geometry/point2d.hpp"

#include "base/geo_object_id.hpp"

#include <memory>
#include <ostream>
#include <stdint.h>
#include <string>
#include <unordered_map>

#include <boost/optional.hpp>

namespace generator
{
namespace geo_objects
{
class StreetsBuilder
{
public:
  StreetsBuilder(RegionInfoGetter const & regionInfoGetter);

  void Build(std::string const & pathInGeoObjectsTmpMwm, std::ostream & streamGeoObjectsKv);

  static bool IsStreet(OsmElement const & element);
  static bool IsStreet(FeatureBuilder1 const & fb);

private:
  using RegionStreets = std::unordered_map<std::string, base::GeoObjectId>;

  void SaveStreetGeoObjects(std::ostream & streamGeoObjectsKv);
  void SaveRegionStreetGeoObjects(std::ostream & streamGeoObjectsKv, uint64_t regionId,
                                  RegionStreets const & streets);

  void AddStreet(FeatureBuilder1 & fb);
  void AddStreetBinding(std::string && streetName, FeatureBuilder1 & fb);
  boost::optional<KeyValue> FindStreetRegionOwner(FeatureBuilder1 & fb);
  boost::optional<KeyValue> FindStreetRegionOwner(m2::PointD const & point);
  bool InsertStreet(KeyValue const & region, std::string && streetName, base::GeoObjectId id);
  bool InsertSurrogateStreet(KeyValue const & region, std::string && streetName);
  std::unique_ptr<char, JSONFreeDeleter> MakeStreetValue(uint64_t regionId, base::Json const regionObject,
                                                         std::string const & streetName);
  base::GeoObjectId NextOsmSurrogateId();

  std::unordered_map<uint64_t, RegionStreets> m_regions;
  RegionInfoGetter const & m_regionInfoGetter;
  uint64_t m_osmSurrogateCounter{0};
};
}  // namespace geo_objects
}  // namespace generator
