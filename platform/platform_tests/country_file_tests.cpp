#include "testing/testing.hpp"

#include "defines.hpp"

#include "platform/mwm_version.hpp"
#include "platform/country_file.hpp"

namespace platform
{
UNIT_TEST(CountryFile_SmokeTwoComponentMwm)
{
  CountryFile countryFile("TestCountry");
  TEST_EQUAL("TestCountry", countryFile.GetName(), ());
  string const mapFileName = GetFileName(countryFile.GetName(), MapOptions::Map,
                                         version::FOR_TESTING_TWO_COMPONENT_MWM1);
  TEST_EQUAL("TestCountry" DATA_FILE_EXTENSION, mapFileName, ());
  string const routingFileName = GetFileName(countryFile.GetName(), MapOptions::CarRouting,
                                             version::FOR_TESTING_TWO_COMPONENT_MWM1);
  TEST_EQUAL("TestCountry" DATA_FILE_EXTENSION ROUTING_FILE_EXTENSION, routingFileName, ());

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

UNIT_TEST(CountryFile_SmokeOneComponentMwm)
{
  CountryFile countryFile("TestCountryOneComponent");
  TEST_EQUAL("TestCountryOneComponent", countryFile.GetName(), ());
  string const mapFileName = GetFileName(countryFile.GetName(), MapOptions::MapWithCarRouting,
                                         version::FOR_TESTING_SINGLE_MWM1);

  TEST_EQUAL("TestCountryOneComponent" DATA_FILE_EXTENSION, mapFileName, ());

  TEST_EQUAL(0, countryFile.GetRemoteSize(MapOptions::Nothing), ());
  TEST_EQUAL(0, countryFile.GetRemoteSize(MapOptions::Map), ());
  TEST_EQUAL(0, countryFile.GetRemoteSize(MapOptions::CarRouting), ());
  TEST_EQUAL(0, countryFile.GetRemoteSize(MapOptions::MapWithCarRouting), ());

  countryFile.SetRemoteSizes(3 /* mapSize */, 0 /* routingSize */);

  TEST_EQUAL(0, countryFile.GetRemoteSize(MapOptions::Nothing), ());
  TEST_EQUAL(3, countryFile.GetRemoteSize(MapOptions::Map), ());
  TEST_EQUAL(0, countryFile.GetRemoteSize(MapOptions::CarRouting), ());
  TEST_EQUAL(3, countryFile.GetRemoteSize(MapOptions::MapWithCarRouting), ());
}
}  // namespace platform
