#include "testing/testing.hpp"

#include "defines.hpp"

#include "platform/mwm_version.hpp"
#include "platform/country_file.hpp"

#include <string>

using namespace std;

namespace platform
{
UNIT_TEST(CountryFile_SmokeTwoComponentMwm)
{
  CountryFile countryFile("TestCountry");
  TEST_EQUAL("TestCountry", countryFile.GetName(), ());
  string const mapFileName = GetFileName(countryFile.GetName(), MapFileType::Map,
                                         version::FOR_TESTING_TWO_COMPONENT_MWM1);
  TEST_EQUAL("TestCountry" DATA_FILE_EXTENSION, mapFileName, ());

  TEST_EQUAL(0, countryFile.GetRemoteSize(), ());

  countryFile.SetRemoteSize(1 /* mapSize */);

  TEST_EQUAL(1, countryFile.GetRemoteSize(), ());
}

UNIT_TEST(CountryFile_SmokeOneComponentMwm)
{
  CountryFile countryFile("TestCountryOneComponent");
  TEST_EQUAL("TestCountryOneComponent", countryFile.GetName(), ());
  string const mapFileName = GetFileName(countryFile.GetName(), MapFileType::Map,
                                         version::FOR_TESTING_SINGLE_MWM1);

  TEST_EQUAL("TestCountryOneComponent" DATA_FILE_EXTENSION, mapFileName, ());

  TEST_EQUAL(0, countryFile.GetRemoteSize(), ());

  countryFile.SetRemoteSize(3 /* mapSize */);

  TEST_EQUAL(3, countryFile.GetRemoteSize(), ());
}
}  // namespace platform
