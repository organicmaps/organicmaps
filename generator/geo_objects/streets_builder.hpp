#pragma once

#include "generator/feature_builder.hpp"
#include "generator/geo_objects/key_value_storage.hpp"
#include "generator/geo_objects/region_info_getter.hpp"
#include "generator/osm_element.hpp"

#include "coding/reader.hpp"

#include "geometry/point2d.hpp"

#include <memory>
#include <ostream>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

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
  boost::optional<KeyValue> FindStreetRegionOwner(FeatureBuilder1 & fb);
  boost::optional<KeyValue> FindStreetRegionOwner(m2::PointD const & point);
  bool InsertStreet(KeyValue const & region, FeatureBuilder1 & fb);
  std::unique_ptr<char, JSONFreeDeleter> MakeStreetValue(FeatureBuilder1 const & fb, KeyValue const & region);

  std::unordered_map<uint64_t, std::unordered_set<std::string>> m_regionsStreets;
  RegionInfoGetter const & m_regionInfoGetter;
};
}  // namespace geo_objects
}  // namespace generator
