#include "testing/testing.hpp"

#include "generator/srtm_parser.hpp"

using namespace generator;

namespace
{
inline std::string GetBase(ms::LatLon const & coord) { return SrtmTile::GetBase(coord); }

UNIT_TEST(FilenameTests)
{
  auto name = GetBase({56.4566, 37.3467});
  TEST_EQUAL(name, "N56E037", ());

  name = GetBase({34.077433, -118.304569});
  TEST_EQUAL(name, "N34W119", ());

  name = GetBase({0.1, 0.1});
  TEST_EQUAL(name, "N00E000", ());

  name = GetBase({-35.35, -12.1});
  TEST_EQUAL(name, "S36W013", ());

  name = GetBase({-34.622358, -58.383654});
  TEST_EQUAL(name, "S35W059", ());
}
}  // namespace
