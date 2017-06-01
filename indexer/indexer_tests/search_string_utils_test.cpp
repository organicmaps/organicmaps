#include "testing/testing.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/string_utils.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace search;
using namespace std;
using namespace strings;

namespace
{
class Utf8StreetTokensFilter
{
public:
  Utf8StreetTokensFilter(vector<pair<string, size_t>> & cont)
    : m_cont(cont)
    , m_filter([&](UniString const & token, size_t tag)
               {
                 m_cont.emplace_back(ToUtf8(token), tag);
               })
  {
  }

  inline void Put(string const & token, bool isPrefix, size_t tag)
  {
    m_filter.Put(MakeUniString(token), isPrefix, tag);
  }

private:
  vector<pair<string, size_t>> & m_cont;
  StreetTokensFilter m_filter;
};

bool TestStreetSynonym(char const * s)
{
  return IsStreetSynonym(MakeUniString(s));
}

bool TestStreetPrefixMatch(char const * s)
{
  return IsStreetSynonymPrefix(MakeUniString(s));
}

string NormalizeAndSimplifyStringUtf8(string const & s)
{
  return strings::ToUtf8(NormalizeAndSimplifyString(s));
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

UNIT_TEST(StreetSynonym)
{
  TEST(TestStreetSynonym("street"), ());
  TEST(TestStreetSynonym("улица"), ());
  TEST(TestStreetSynonym("strasse"), ());
  TEST(!TestStreetSynonym("strase"), ());
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

UNIT_TEST(StreetTokensFilter)
{
  using TList = vector<pair<string, size_t>>;

  {
    TList expected = {};
    TList actual;

    Utf8StreetTokensFilter filter(actual);
    filter.Put("ули", true /* isPrefix */, 0 /* tag */);

    TEST_EQUAL(expected, actual, ());
  }

  {
    TList expected = {};
    TList actual;

    Utf8StreetTokensFilter filter(actual);
    filter.Put("улица", false /* isPrefix */, 0 /* tag */);

    TEST_EQUAL(expected, actual, ());
  }

  {
    TList expected = {{"генерала", 1}, {"антонова", 2}};
    TList actual;

    Utf8StreetTokensFilter filter(actual);
    filter.Put("ул", false /* isPrefix */, 0 /* tag */);
    filter.Put("генерала", false /* isPrefix */, 1 /* tag */);
    filter.Put("антонова", false /* isPrefix */, 2 /* tag */);

    TEST_EQUAL(expected, actual, ());
  }

  {
    TList expected = {{"улица", 100}, {"набережная", 50}};
    TList actual;

    Utf8StreetTokensFilter filter(actual);
    filter.Put("улица", false /* isPrefix */, 100 /* tag */);
    filter.Put("набережная", true /* isPrefix */, 50 /* tag */);

    TEST_EQUAL(expected, actual, ());
  }

  {
    TList expected = {{"улица", 0}, {"набережная", 1}, {"проспект", 2}};
    TList actual;

    Utf8StreetTokensFilter filter(actual);
    filter.Put("улица", false /* isPrefix */, 0 /* tag */);
    filter.Put("набережная", true /* isPrefix */, 1 /* tag */);
    filter.Put("проспект", false /* isPrefix */, 2 /* tag */);

    TEST_EQUAL(expected, actual, ());
  }
}

UNIT_TEST(NormalizeAndSimplifyString_Numero)
{
  TEST_EQUAL(NormalizeAndSimplifyStringUtf8("Зона №51"), "зона  51", ());
  TEST_EQUAL(NormalizeAndSimplifyStringUtf8("Зона № 51"), "зона   51", ());
  TEST_EQUAL(NormalizeAndSimplifyStringUtf8("Area #51"), "area  51", ());
  TEST_EQUAL(NormalizeAndSimplifyStringUtf8("Area # "), "area   ", ());
  TEST_EQUAL(NormalizeAndSimplifyStringUtf8("Area #One"), "area #one", ());
}
