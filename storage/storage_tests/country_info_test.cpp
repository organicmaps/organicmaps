#include "../../testing/testing.hpp"

#include "../country_info.hpp"
#include "../country.hpp"

#include "../../indexer/mercator.hpp"

#include "../../platform/platform.hpp"


UNIT_TEST(CountryInfo_GetByPoint_Smoke)
{
  Platform & pl = GetPlatform();

  storage::CountryInfoGetter getter(pl.GetReader(PACKED_POLYGONS_FILE),
                                    pl.GetReader(COUNTRIES_FILE));

  // Minsk
  TEST_EQUAL(getter.GetRegionName(
               m2::PointD(MercatorBounds::LonToX(27.5618818),
                          MercatorBounds::LatToY(53.9022651))), "Belarus", ());
}

UNIT_TEST(CountryInfo_ValidName_Smoke)
{
  string buffer;
  ReaderPtr<Reader>(GetPlatform().GetReader(COUNTRIES_FILE)).ReadAsString(buffer);

  map<string, string> id2name;
  storage::LoadCountryFile2Name(buffer, id2name);

  TEST(id2name.count("Germany_Baden-Wurttemberg") == 1, ());
  TEST(id2name.count("France_Paris & Ile-de-France") == 1, ());

  TEST(id2name.count("Russia_Far Eastern") == 0, ());
  TEST(id2name.count("UK_Northern Ireland") == 0, ());
}
