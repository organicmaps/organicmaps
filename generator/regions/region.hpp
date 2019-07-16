#pragma once

#include "generator/feature_builder.hpp"
#include "generator/regions/place_point.hpp"
#include "generator/regions/region_base.hpp"

#include <memory>

namespace feature
{
class FeatureBuilder;
}  // namespace feature

namespace generator
{
class RegionDataProxy;

namespace regions
{
// This is a helper class that is needed to represent the region.
// With this view, further processing is simplified.
class Region : protected RegionWithName, protected RegionWithData
{
public:
  explicit Region(feature::FeatureBuilder const & fb, RegionDataProxy const & rd);
  Region(StringUtf8Multilang const & name, RegionDataProxy const & rd,
         std::shared_ptr<BoostPolygon> const & polygon);

  // See RegionWithName::GetTranslatedOrTransliteratedName().
  std::string GetTranslatedOrTransliteratedName(LanguageCode languageCode) const;
  std::string GetInternationalName() const;
  std::string GetName(int8_t lang = StringUtf8Multilang::kDefaultCode) const;

  base::GeoObjectId GetId() const;
  using RegionWithData::GetAdminLevel;
  PlaceType GetPlaceType() const;
  boost::optional<std::string> GetIsoCode() const;

  using RegionWithData::GetLabelOsmId;
  boost::optional<PlacePoint> const & GetLabel() const noexcept;
  void SetLabel(PlacePoint const & place);

  bool Contains(Region const & smaller) const;
  bool ContainsRect(Region const & smaller) const;
  bool Contains(PlacePoint const & place) const;
  bool Contains(BoostPoint const & point) const;
  double CalculateOverlapPercentage(Region const & other) const;
  BoostPoint GetCenter() const;
  bool IsLocality() const;
  BoostRect const & GetRect() const { return m_rect; }
  std::shared_ptr<BoostPolygon> const & GetPolygon() const noexcept { return m_polygon; }
  void SetPolygon(std::shared_ptr<BoostPolygon> const & polygon);
  double GetArea() const { return m_area; }

private:
  void FillPolygon(feature::FeatureBuilder const & fb);

  boost::optional<PlacePoint> m_placeLabel;
  std::shared_ptr<BoostPolygon> m_polygon;
  BoostRect m_rect;
  double m_area;
};
}  // namespace regions
}  // namespace generator
