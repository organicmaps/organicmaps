#include "testing/testing.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/string_utils.hpp"

using namespace search;
using namespace strings;

namespace
{
bool TestStreetPrefixMatch(char const * s)
{
  return IsStreetSynonymPrefix(MakeUniString(s));
}
} // namespace

UNIT_TEST(FeatureTypeToString)
{
  TEST_EQUAL("!type:123", ToUtf8(FeatureTypeToString(123)), ());
}

UNIT_TEST(NormalizeAndSimplifyStringWithOurTambourines)
{
  // This test is dependent from strings::NormalizeAndSimplifyString implementation.
  // TODO: Fix it when logic with и-й will change.

  /*
  string const arr[] = {"ÜbërÅłłęšß", "uberallesss", // Basic test case.
                        "Iiİı", "iiii",              // Famous turkish "I" letter bug.
                        "ЙЁйёШКИЙй", "йейешкийй",    // Better handling of Russian й letter.
                        "ØøÆæŒœ", "ooaeaeoeoe",
                        "バス", "ハス",
                        "âàáạăốợồôểềệếỉđưựứửýĂÂĐÊÔƠƯ",
                        "aaaaaooooeeeeiduuuuyaadeoou",  // Vietnamese
                        "ăâț", "aat"                    // Romanian
                       };
  */
  string const arr[] = {"ÜbërÅłłęšß", "uberallesss", // Basic test case.
                        "Iiİı", "iiii",              // Famous turkish "I" letter bug.
                        "ЙЁйёШКИЙй", "иеиешкиии",    // Better handling of Russian й letter.
                        "ØøÆæŒœ", "ooaeaeoeoe",      // Dansk
                        "バス", "ハス",
                        "âàáạăốợồôểềệếỉđưựứửýĂÂĐÊÔƠƯ",
                        "aaaaaooooeeeeiduuuuyaadeoou",  // Vietnamese
                        "ăâț", "aat",                   // Romanian
                        "Триу́мф-Пала́с", "триумф-палас", // Russian accent
                       };

  for (size_t i = 0; i < ARRAY_SIZE(arr); i += 2)
    TEST_EQUAL(arr[i + 1], ToUtf8(NormalizeAndSimplifyString(arr[i])), (i));
}

UNIT_TEST(Contains)
{
  constexpr char const * kTestStr = "ØøÆæŒœ Ўвага!";
  TEST(ContainsNormalized(kTestStr, ""), ());
  TEST(!ContainsNormalized("", "z"), ());
  TEST(ContainsNormalized(kTestStr, "ooae"), ());
  TEST(ContainsNormalized(kTestStr, " у"), ());
  TEST(ContainsNormalized(kTestStr, "Ў"), ());
  TEST(ContainsNormalized(kTestStr, "ўв"), ());
  TEST(!ContainsNormalized(kTestStr, "ага! "), ());
  TEST(!ContainsNormalized(kTestStr, "z"), ());
}

UNIT_TEST(StreetPrefixMatch)
{
  TEST(TestStreetPrefixMatch("п"), ());
  TEST(TestStreetPrefixMatch("пр"), ());
  TEST(TestStreetPrefixMatch("про"), ());
  TEST(TestStreetPrefixMatch("прое"), ());
  TEST(TestStreetPrefixMatch("проез"), ());
  TEST(TestStreetPrefixMatch("проезд"), ());
  TEST(!TestStreetPrefixMatch("проездд"), ());
}
