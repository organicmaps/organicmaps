#include "testing/testing.hpp"

#include "platform/platform.hpp"

#include "coding/sha1.hpp"

#include "base/logging.hpp"

UNIT_TEST(CalculateWorldSHA)
{
  auto const path = GetPlatform().ResourcesDir();
  for (char const * country : {WORLD_FILE_NAME, WORLD_COASTS_FILE_NAME})
    LOG(LINFO, (country, coding::SHA1::CalculateBase64(path + country + DATA_FILE_EXTENSION)));
}
