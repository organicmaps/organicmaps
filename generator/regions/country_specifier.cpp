#include "generator/regions/country_specifier.hpp"

namespace generator
{
namespace regions
{
void CountrySpecifier::AdjustRegionsLevel(Node::PtrList & outers)
{ }

PlaceLevel CountrySpecifier::GetLevel(Region const & region) const
{
  auto const placeType = region.GetPlaceType();
  auto const placeLevel = GetLevel(placeType);
  if (placeLevel != PlaceLevel::Unknown)
    return placeLevel;

  return GetSpecificCountryLevel(region);
}

PlaceLevel CountrySpecifier::GetSpecificCountryLevel(Region const & region) const
{
  auto const placeType = region.GetPlaceType();
  if (region.GetAdminLevel() == AdminLevel::Two &&
      (placeType == PlaceType::Country || placeType == PlaceType::Unknown))
  {
    return PlaceLevel::Country;
  }

  return PlaceLevel::Unknown;
}

// static
PlaceLevel CountrySpecifier::GetLevel(PlaceType placeType)
{
  switch (placeType)
  {
  case PlaceType::Country:
    return PlaceLevel::Country;
  case PlaceType::State:
  case PlaceType::Province:
    return PlaceLevel::Region;
  case PlaceType::District:
  case PlaceType::County:
  case PlaceType::Municipality:
    return PlaceLevel::Subregion;
  case PlaceType::City:
  case PlaceType::Town:
  case PlaceType::Village:
  case PlaceType::Hamlet:
  case PlaceType::IsolatedDwelling:
    return PlaceLevel::Locality;
  case PlaceType::Suburb:
    return PlaceLevel::Suburb;
  case PlaceType::Quarter:
  case PlaceType::Neighbourhood:
    return PlaceLevel::Sublocality;
    return PlaceLevel::Sublocality;
  case PlaceType::Unknown:
    break;
  }

  return PlaceLevel::Unknown;
}

int CountrySpecifier::RelateByWeight(LevelRegion const & l, LevelRegion const & r) const
{
  auto const lLevel = l.GetLevel();
  auto const rLevel = r.GetLevel();
  if (lLevel != PlaceLevel::Unknown && rLevel != PlaceLevel::Unknown)
  {
    if (lLevel > rLevel)
      return -1;
    if (lLevel < rLevel)
      return 1;
  }

  auto const lPlaceType = l.GetPlaceType();
  auto const rPlaceType = r.GetPlaceType();
  if (lPlaceType != PlaceType::Unknown && rPlaceType != PlaceType::Unknown)
  {
    if (lPlaceType > rPlaceType)
      return -1;
    if (lPlaceType < rPlaceType)
      return 1;
    // Check by admin level (administrative city (district + city) > city).
  }

  auto const lAdminLevel = l.GetAdminLevel();
  auto const rAdminLevel = r.GetAdminLevel();
  if (lAdminLevel != AdminLevel::Unknown && rAdminLevel != AdminLevel::Unknown)
  {
    if (lAdminLevel > rAdminLevel &&
        // Ignore capital point (admin_level=2).
        (rAdminLevel != AdminLevel::Two || rLevel == PlaceLevel::Country))
    {
      return -1;
    }
    if (lAdminLevel < rAdminLevel &&
        // Ignore capital point (admin_level=2).
        (lAdminLevel != AdminLevel::Two || lLevel == PlaceLevel::Country))
    {
      return 1;
    }
  }
  if (lAdminLevel != AdminLevel::Unknown)
    return 1;
  if (rAdminLevel != AdminLevel::Unknown)
    return -1;

  return 0;
}
}  // namespace regions
}  // namespace generator
