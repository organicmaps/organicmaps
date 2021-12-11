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
  {
    CountryFile cf("One");
    TEST_EQUAL("One", cf.GetName(), ());
    string const mapFileName = GetFileName(cf.GetName(), MapFileType::Map);

    TEST_EQUAL("One" DATA_FILE_EXTENSION, mapFileName, ());
    TEST_EQUAL(0, cf.GetRemoteSize(), ());
  }

  {
    CountryFile cf("Three", 666, "xxxSHAxxx");
    TEST_EQUAL("Three", cf.GetName(), ());
    string const mapFileName = GetFileName(cf.GetName(), MapFileType::Map);

    TEST_EQUAL("Three" DATA_FILE_EXTENSION, mapFileName, ());
    TEST_EQUAL(666, cf.GetRemoteSize(), ());
    TEST_EQUAL("xxxSHAxxx", cf.GetSha1(), ());
  }
}
}  // namespace platform
