#pragma once

#include "generator/feature_builder.hpp"
#include "generator/regions/region_info.hpp"
#include "generator/translation.hpp"

#include "geometry/rect2d.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <string>

#include <boost/geometry.hpp>
#include <boost/optional.hpp>

namespace generator
{
namespace regions
{
using Point = feature::FeatureBuilder::PointSeq::value_type;
using BoostPoint = boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>;
using BoostPolygon = boost::geometry::model::polygon<BoostPoint>;
using BoostRect = boost::geometry::model::box<BoostPoint>;

class RegionWithName
{
public:
  explicit RegionWithName(StringUtf8Multilang name) : m_name(std::move(name)) {}

  std::string GetTranslatedOrTransliteratedName(LanguageCode languageCode) const;
  // returns default name if int_name is empty
  std::string GetInternationalName() const;
  std::string GetName(int8_t lang = StringUtf8Multilang::kDefaultCode) const;
  StringUtf8Multilang const & GetMultilangName() const;

protected:
  StringUtf8Multilang m_name;
};

class RegionWithData
{
public:
  explicit RegionWithData(RegionDataProxy const & regionData) : m_regionData(regionData) {}

  base::GeoObjectId GetId() const;
  boost::optional<std::string> GetIsoCode() const;

  boost::optional<base::GeoObjectId> GetLabelOsmId() const;

  AdminLevel GetAdminLevel() const { return m_regionData.GetAdminLevel(); }
  PlaceType GetPlaceType() const { return m_regionData.GetPlaceType(); }

  void SetAdminLevel(AdminLevel adminLevel) { m_regionData.SetAdminLevel(adminLevel); }
  void SetPlaceType(PlaceType placeType) { m_regionData.SetPlaceType(placeType); }

  RegionDataProxy const & GetRegionData() const { return m_regionData; }

protected:
  RegionDataProxy m_regionData;
};

template <typename Place>
std::string GetRegionNotation(Place const & place)
{
  auto notation = place.GetTranslatedOrTransliteratedName(StringUtf8Multilang::GetLangIndex("en"));
  if (notation.empty())
    return place.GetName();

  if (notation != place.GetName())
    notation += " / " + place.GetName();
  return notation;
}
}  // namespace regions
}  // namespace generator
