#include "testing/testing.hpp"

#include "platform/platform.hpp"

#include "coding/blake3.hpp"

#include "base/logging.hpp"

#include "defines.hpp"

UNIT_TEST(CalculateWorldHash)
{
  auto const path = GetPlatform().ResourcesDir();
  for (char const * country : {WORLD_FILE_NAME, WORLD_COASTS_FILE_NAME})
    LOG(LINFO, (country, coding::Blake3::CalculateMwmBase64(path + country + DATA_FILE_EXTENSION)));
}
