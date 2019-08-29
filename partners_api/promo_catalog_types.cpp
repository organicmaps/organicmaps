#include "partners_api/promo_catalog_types.hpp"

namespace
{
promo::TypesList kTypes = {
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
  {"tourism", "viewpoint"}
};
}  // namespace

namespace promo
{
TypesList const & GetPromoCatalogPoiTypes()
{
  return kTypes;
}
}  // namespace promo
