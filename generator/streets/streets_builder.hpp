#pragma once

#include "generator/feature_builder.hpp"
#include "generator/key_value_storage.hpp"
#include "generator/osm_element.hpp"
#include "generator/regions/region_info_getter.hpp"
#include "generator/streets/street_geometry.hpp"

#include "coding/reader.hpp"

#include "geometry/point2d.hpp"

#include "base/geo_object_id.hpp"

#include <stdint.h>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <unordered_map>

#include <boost/optional.hpp>

namespace generator
{
namespace streets
{
class StreetsBuilder
{
public:
  explicit StreetsBuilder(regions::RegionInfoGetter const & regionInfoGetter, size_t threadsCount);

  void AssembleStreets(std::string const & pathInStreetsTmpMwm);
  void AssembleBindings(std::string const & pathInGeoObjectsTmpMwm);
  // Save built streets in the jsonl format with the members: "properties", "bbox" (array: left
  // bottom longitude, left bottom latitude, right top longitude, right top latitude), "pin" (array:
  // longitude, latitude).
  void SaveStreetsKv(std::ostream & streamStreetsKv);

  static bool IsStreet(OsmElement const & element);
  static bool IsStreet(feature::FeatureBuilder const & fb);

private:
  struct Street
  {
    StringUtf8Multilang m_name;
    StreetGeometry m_geometry;
  };
  using RegionStreets = std::unordered_map<std::string, Street>;

  void SaveRegionStreetsKv(std::ostream & streamStreetsKv, uint64_t regionId,
                           RegionStreets const & streets);

  void AddStreet(feature::FeatureBuilder & fb);
  void AddStreetHighway(feature::FeatureBuilder & fb);
  void AddStreetArea(feature::FeatureBuilder & fb);
  void AddStreetPoint(feature::FeatureBuilder & fb);
  void AddStreetBinding(std::string && streetName, feature::FeatureBuilder & fb,
                        StringUtf8Multilang const & multiLangName);
  boost::optional<KeyValue> FindStreetRegionOwner(m2::PointD const & point,
                                                  bool needLocality = false);
  Street & InsertStreet(uint64_t regionId, std::string && streetName,
                        StringUtf8Multilang const & multilangName);
  base::JSONPtr MakeStreetValue(uint64_t regionId, JsonValue const & regionObject,
                                const StringUtf8Multilang & streetName, m2::RectD const & bbox,
                                m2::PointD const & pinPoint);
  base::GeoObjectId NextOsmSurrogateId();

  std::unordered_map<uint64_t, RegionStreets> m_regions;
  regions::RegionInfoGetter const & m_regionInfoGetter;
  uint64_t m_osmSurrogateCounter{0};
  size_t m_threadsCount;
  std::mutex m_updateMutex;
};
}  // namespace streets
}  // namespace generator
