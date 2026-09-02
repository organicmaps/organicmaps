#include "testing/testing.hpp"

#include "map/terrain_provider.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include <string_view>

namespace terrain_gate_tests
{
// The terrain draws where the map does: a land tile needs its region downloaded (the
// "Download" call to action shows there otherwise), the open ocean has no region to
// download and always draws its bathymetry.
UNIT_TEST(TerrainGate_DrawsWhereTheMapIs)
{
  auto const infoGetter = storage::CountryInfoReader::CreateCountryInfoGetter(GetPlatform());
  auto const notLoaded = [](std::string_view) { return false; };

  m2::PointD const munich = mercator::FromLatLon(48.137, 11.575);
  storage::CountryId const region = infoGetter->GetRegionCountryId(munich);
  TEST(!region.empty(), ());
  auto const loaded = [&region](std::string_view name) { return name == region; };
  TEST(terrain::IsTerrainDrawableAt(*infoGetter, loaded, munich), (region));
  TEST(!terrain::IsTerrainDrawableAt(*infoGetter, notLoaded, munich), (region));

  m2::PointD const atlantic = mercator::FromLatLon(30.0, -40.0);
  TEST(infoGetter->GetRegionCountryId(atlantic).empty(), ());
  TEST(terrain::IsTerrainDrawableAt(*infoGetter, notLoaded, atlantic), ());
}
}  // namespace terrain_gate_tests
