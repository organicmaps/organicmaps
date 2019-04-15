#pragma once

#include "generator/feature_builder.hpp"
#include "generator/regions/region_info.hpp"

#include "geometry/rect2d.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <string>

#include <boost/geometry.hpp>

namespace generator
{
namespace regions
{
using Point = FeatureBuilder1::PointSeq::value_type;
using BoostPoint = boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>;
using BoostPolygon = boost::geometry::model::polygon<BoostPoint>;
using BoostRect = boost::geometry::model::box<BoostPoint>;

class RegionWithName
{
public:
  RegionWithName(StringUtf8Multilang const & name) : m_name(name) {}

  // This function will take the following steps:
  // 1. Return the english name if it exists.
  // 2. Return transliteration if it succeeds.
  // 3. Otherwise, return empty string.
  std::string GetEnglishOrTransliteratedName() const;
  std::string GetName(int8_t lang = StringUtf8Multilang::kDefaultCode) const;
  StringUtf8Multilang const & GetMultilangName() const;
  void SetMultilangName(StringUtf8Multilang const & name);

protected:
  StringUtf8Multilang m_name;
};

class RegionWithData
{
public:
  RegionWithData(RegionDataProxy const & regionData) : m_regionData(regionData) {}

  base::GeoObjectId GetId() const;
  bool HasIsoCode() const;
  std::string GetIsoCode() const;

  AdminLevel GetAdminLevel() const { return m_regionData.GetAdminLevel(); }
  PlaceType GetPlaceType() const { return m_regionData.GetPlaceType(); }

  void SetAdminLevel(AdminLevel adminLevel) { m_regionData.SetAdminLevel(adminLevel); }
  void SetPlaceType(PlaceType placeType) { m_regionData.SetPlaceType(placeType); }

  RegionDataProxy const & GetRegionData() const { return m_regionData; }

protected:
  RegionDataProxy m_regionData;
};
}  // namespace regions
}  // namespace generator
