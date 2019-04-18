#include "generator/geo_objects/geo_objects_filter.hpp"

#include "generator/geo_objects/streets_builder.hpp"
#include "generator/osm_element_helpers.hpp"

#include "indexer/ftypes_matcher.hpp"

namespace generator
{
namespace geo_objects
{
bool GeoObjectsFilter::IsAccepted(OsmElement const & element)
{
  return osm_element::IsBuilding(element) || osm_element::HasHouse(element) || osm_element::IsPoi(element) ||
         geo_objects::StreetsBuilder::IsStreet(element);
}

bool GeoObjectsFilter::IsAccepted(FeatureBuilder1 const & feature)
{
  if (!feature.GetParams().IsValid())
    return false;
  
  return IsBuilding(feature) || HasHouse(feature) || IsPoi(feature) || IsStreet(feature);
}

// static
bool GeoObjectsFilter::IsBuilding(FeatureBuilder1 const & fb)
{
  auto const & checker = ftypes::IsBuildingChecker::Instance();
  return checker(fb.GetTypes());
}

// static
bool GeoObjectsFilter::HasHouse(FeatureBuilder1 const & fb)
{
  return !fb.GetParams().house.IsEmpty();
}

// static
bool GeoObjectsFilter::IsPoi(FeatureBuilder1 const & fb)
{
  auto const & poiChecker = ftypes::IsPoiChecker::Instance();
  return poiChecker(fb.GetTypes());
}

// static
bool GeoObjectsFilter::IsStreet(FeatureBuilder1 const & fb)
{
  return geo_objects::StreetsBuilder::IsStreet(fb);
}
}  // namespace geo_objects
}  // namespace generator
