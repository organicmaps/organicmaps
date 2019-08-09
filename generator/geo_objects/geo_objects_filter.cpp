#include "generator/geo_objects/geo_objects_filter.hpp"

#include "generator/osm_element_helpers.hpp"

#include "indexer/ftypes_matcher.hpp"

using namespace feature;

namespace generator
{
namespace geo_objects
{
std::shared_ptr<FilterInterface> GeoObjectsFilter::Clone() const
{
  return std::make_shared<GeoObjectsFilter>();
}

bool GeoObjectsFilter::IsAccepted(OsmElement const & element)
{
  return osm_element::IsBuilding(element) || osm_element::HasHouse(element) || osm_element::IsPoi(element);
}

bool GeoObjectsFilter::IsAccepted(FeatureBuilder const & feature)
{
  if (!feature.GetParams().IsValid())
    return false;

  if (feature.IsLine())
    return false;

  return IsBuilding(feature) || HasHouse(feature) || IsPoi(feature);
}

// static
bool GeoObjectsFilter::IsBuilding(FeatureBuilder const & fb)
{
  auto const & checker = ftypes::IsBuildingChecker::Instance();
  return checker(fb.GetTypes());
}

// static
bool GeoObjectsFilter::HasHouse(FeatureBuilder const & fb)
{
  return !fb.GetParams().house.IsEmpty();
}

// static
bool GeoObjectsFilter::IsPoi(FeatureBuilder const & fb)
{
  auto const & poiChecker = ftypes::IsPoiChecker::Instance();
  return poiChecker(fb.GetTypes());
}
}  // namespace geo_objects
}  // namespace generator
