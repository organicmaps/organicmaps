#include "testing/testing.hpp"

#include "defines.hpp"
#include "platform/country_file.hpp"

namespace platform
{
UNIT_TEST(CountryFile_Smoke)
{
  CountryFile countryFile("TestCountry");
  TEST_EQUAL("TestCountry", countryFile.GetNameWithoutExt(), ());
  TEST_EQUAL("TestCountry" DATA_FILE_EXTENSION, countryFile.GetNameWithExt(MapOptions::Map), ());
  TEST_EQUAL("TestCountry" DATA_FILE_EXTENSION ROUTING_FILE_EXTENSION,
             countryFile.GetNameWithExt(MapOptions::CarRouting), ());

  TEST_EQUAL(0, countryFile.GetRemoteSize(MapOptions::Nothing), ());
  TEST_EQUAL(0, countryFile.GetRemoteSize(MapOptions::Map), ());
  TEST_EQUAL(0, countryFile.GetRemoteSize(MapOptions::CarRouting), ());
  TEST_EQUAL(0, countryFile.GetRemoteSize(MapOptions::MapWithCarRouting), ());

  countryFile.SetRemoteSizes(1 /* mapSize */, 2 /* routingSize */);

  TEST_EQUAL(0, countryFile.GetRemoteSize(MapOptions::Nothing), ());
  TEST_EQUAL(1, countryFile.GetRemoteSize(MapOptions::Map), ());
  TEST_EQUAL(2, countryFile.GetRemoteSize(MapOptions::CarRouting), ());
  TEST_EQUAL(3, countryFile.GetRemoteSize(MapOptions::MapWithCarRouting), ());
}
}  // namespace platform
