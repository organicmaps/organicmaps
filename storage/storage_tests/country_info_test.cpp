#include "../../testing/testing.hpp"

#include "../country_info.hpp"

#include "../../indexer/mercator.hpp"

#include "../../platform/platform.hpp"


UNIT_TEST(CountryInfo_GetByPoint_Smoke)
{
  storage::CountryInfoGetter getter(GetPlatform().GetReader(PACKED_POLYGONS_FILE));

  // Minsk
  TEST_EQUAL(getter.GetRegionName(
               m2::PointD(MercatorBounds::LonToX(27.5618818),
                          MercatorBounds::LatToY(53.9022651))), "Belarus", ());
}
