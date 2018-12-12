#pragma once

#include "generator/feature_builder.hpp"
#include "generator/regions/collector_region_info.hpp"

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

struct RegionWithName
{
  RegionWithName(StringUtf8Multilang const & name) : m_name(name) {}

  // This function will take the following steps:
  // 1. Return the english name if it exists.
  // 2. Return transliteration if it succeeds.
  // 3. Otherwise, return empty string.
  std::string GetEnglishOrTransliteratedName() const;
  std::string GetName(int8_t lang = StringUtf8Multilang::kDefaultCode) const;
  StringUtf8Multilang const & GetStringUtf8MultilangName() const;
  void SetStringUtf8MultilangName(StringUtf8Multilang const & name);

protected:
  StringUtf8Multilang m_name;
};

struct RegionWithData
{
  static uint8_t constexpr kNoRank = 0;

  RegionWithData(RegionDataProxy const & regionData) : m_regionData(regionData) {}

  base::GeoObjectId GetId() const;
  bool HasAdminCenter() const;
  base::GeoObjectId GetAdminCenterId() const;
  bool HasIsoCode() const;
  std::string GetIsoCode() const;

  // Absolute rank values do not mean anything. But if the rank of the first object is more than the
  // rank of the second object, then the first object is considered more nested.
  uint8_t GetRank() const;
  std::string GetLabel() const;

  AdminLevel GetAdminLevel() const { return m_regionData.GetAdminLevel(); }
  PlaceType GetPlaceType() const { return m_regionData.GetPlaceType(); }

  void SetAdminLevel(AdminLevel adminLevel) { m_regionData.SetAdminLevel(adminLevel); }
  void SetPlaceType(PlaceType placeType) { m_regionData.SetPlaceType(placeType); }

  bool HasAdminLevel() const { return m_regionData.HasAdminLevel(); }
  bool HasPlaceType() const { return m_regionData.HasPlaceType(); }

protected:
  RegionDataProxy m_regionData;
};
}  // namespace regions
}  // namespace generator
