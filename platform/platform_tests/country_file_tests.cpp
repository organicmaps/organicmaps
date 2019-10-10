#include "testing/testing.hpp"

#include "defines.hpp"

#include "platform/mwm_version.hpp"
#include "platform/country_file.hpp"

#include <string>

using namespace std;

namespace platform
{
UNIT_TEST(CountryFile_Smoke)
{
  CountryFile countryFile("TestCountryOneComponent");
  TEST_EQUAL("TestCountryOneComponent", countryFile.GetName(), ());
  string const mapFileName =
      GetFileName(countryFile.GetName(), MapFileType::Map);

  TEST_EQUAL("TestCountryOneComponent" DATA_FILE_EXTENSION, mapFileName, ());

  TEST_EQUAL(0, countryFile.GetRemoteSize(), ());

  countryFile.SetRemoteSize(3 /* mapSize */);

  TEST_EQUAL(3, countryFile.GetRemoteSize(), ());
}
}  // namespace platform
