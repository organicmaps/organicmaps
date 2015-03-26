#include "../../testing/testing.hpp"
#include "../search_string_utils.hpp"

#include "../../base/string_utils.hpp"

UNIT_TEST(FeatureTypeToString)
{
  TEST_EQUAL("!type:123", strings::ToUtf8(search::FeatureTypeToString(123)), ());
}

UNIT_TEST(NormalizeAndSimplifyStringWithOurTambourines)
{
  string const arr[] = {"ÜbërÅłłęšß", "uberallesss", // Basic test case.
                        "Iiİı", "iiii",              // Famous turkish "I" letter bug.
                        "ЙЁйёШКИЙй", "йейешкийй",    // Better handling of Russian й letter.
                        "ØøÆæŒœ", "ooaeaeoeoe",
                        "バス", "ハス"
                       };
  for (size_t i = 0; i < ARRAY_SIZE(arr); i += 2)
    TEST_EQUAL(arr[i + 1], strings::ToUtf8(search::NormalizeAndSimplifyString(arr[i])), (i));
}
