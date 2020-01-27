#include "partners_api/promo_catalog_types.hpp"

namespace
{
promo::TypesList kSightseeingsTypes = {
  {"tourism", "gallery"},
  {"tourism", "museum"},
  {"amenity", "arts_centre"},
  {"historic", "monument"},
  {"historic", "memorial"},
  {"tourism", "attraction"},
  {"historic", "fort"},
  {"historic", "castle"},
  {"tourism", "artwork"},
  {"historic", "ruins"},
  {"leisure", "park"},
  {"leisure", "garden"},
  {"tourism", "theme_park"},
  {"leisure", "water_park"},
  {"amenity", "nightclub"},
  {"amenity", "cinema"},
  {"amenity", "theatre"},
  {"tourism", "viewpoint"},
  {"amenity", "place_of_worship"},
};

promo::TypesList kOutdoorTypes = {
  {"place", "village"},
  {"place", "hamlet"},
  {"natural", "volcano"},
  {"waterway", "waterfall"},
  {"natural", "cave_entrance"},
  {"landuse", "forest"},
  {"leisure", "nature_reserve"},
  {"natural", "cliff"},
  {"natural", "peak"},
  {"natural", "rock"},
  {"natural", "bare_rock"},
  {"natural", "glacier"},
  {"boundary", "national_park"}
};

}  // namespace

namespace promo
{
TypesList const & GetPromoCatalogSightseeingsTypes()
{
  return kSightseeingsTypes;
}

TypesList const & GetPromoCatalogOutdoorTypes()
{
  return kOutdoorTypes;
}
}  // namespace promo
