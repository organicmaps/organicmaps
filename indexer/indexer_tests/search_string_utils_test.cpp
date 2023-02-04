#include "testing/testing.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/string_utils.hpp"

#include <string>
#include <vector>

namespace search_string_utils_test
{
using namespace search;
using namespace std;
using namespace strings;

class Utf8StreetTokensFilter
{
public:
  explicit Utf8StreetTokensFilter(vector<pair<string, size_t>> & cont, bool withMisprints = false)
    : m_cont(cont)
    , m_filter(
          [&](UniString const & token, size_t tag) { m_cont.emplace_back(ToUtf8(token), tag); },
          withMisprints)
  {
  }

  void Put(string const & token, bool isPrefix, size_t tag)
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

bool TestStreetSynonymWithMisprints(char const * s)
{
  return IsStreetSynonymWithMisprints(MakeUniString(s));
}

bool TestStreetPrefixMatchWithMisprints(char const * s)
{
  return IsStreetSynonymPrefixWithMisprints(MakeUniString(s));
}

string NormalizeAndSimplifyStringUtf8(string const & s)
{
  return strings::ToUtf8(NormalizeAndSimplifyString(s));
}

UNIT_TEST(FeatureTypeToString)
{
  TEST_EQUAL("!type:123", ToUtf8(FeatureTypeToString(123)), ());
}

UNIT_TEST(NormalizeAndSimplifyString_WithOurTambourines)
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
                        "  a   b  c d ", " a b c d ",   // Multiple spaces
                       };

  for (size_t i = 0; i < ARRAY_SIZE(arr); i += 2)
    TEST_EQUAL(arr[i + 1], NormalizeAndSimplifyStringUtf8(arr[i]), ());
}

UNIT_TEST(NormalizeAndSimplifyString_Contains)
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

UNIT_TEST(Street_Synonym)
{
  TEST(TestStreetSynonym("street"), ());
  TEST(TestStreetSynonym("улица"), ());

  TEST(TestStreetSynonym("strasse"), ());
  TEST(TestStreetSynonymWithMisprints("strasse"), ());
  TEST(!TestStreetSynonym("strase"), ());
  TEST(TestStreetSynonymWithMisprints("strase"), ());

//  TEST(TestStreetSynonym("boulevard"), ());
//  TEST(TestStreetSynonymWithMisprints("boulevard"), ());
//  TEST(!TestStreetSynonym("boulevrd"), ());
//  TEST(TestStreetSynonymWithMisprints("boulevrd"), ());

  TEST(TestStreetSynonym("avenue"), ());
  TEST(TestStreetSynonymWithMisprints("avenue"), ());
  TEST(!TestStreetSynonym("aveneu"), ());
  TEST(TestStreetSynonymWithMisprints("aveneu"), ());

  TEST(!TestStreetSynonymWithMisprints("abcdefg"), ());

  TEST(TestStreetSynonym("g."), ());
  TEST(TestStreetSynonymWithMisprints("g."), ());

  TEST(!TestStreetSynonymWithMisprints("ву"), ());
  TEST(TestStreetSynonymWithMisprints("вул"), ());

  TEST(!TestStreetSynonymWithMisprints("gat"), ());
  TEST(!TestStreetSynonymWithMisprints("sok"), ());
  TEST(!TestStreetSynonymWithMisprints("ca"), ());

  // soka -> sokak
  TEST(TestStreetSynonymWithMisprints("soka"), ());
  TEST(!TestStreetSynonym("soka"), ());
}

UNIT_TEST(Street_PrefixMatch)
{
  TEST(TestStreetPrefixMatch("у"), ());
  TEST(TestStreetPrefixMatch("ул"), ());
  TEST(TestStreetPrefixMatch("ули"), ());

  TEST(TestStreetPrefixMatch("gat"), ());
  TEST(TestStreetPrefixMatch("sok"), ());
  TEST(TestStreetPrefixMatch("ca"), ());
  TEST(TestStreetPrefixMatch("ву"), ());

//  TEST(TestStreetPrefixMatch("п"), ());
//  TEST(TestStreetPrefixMatch("пр"), ());
//  TEST(TestStreetPrefixMatch("про"), ());
//  TEST(TestStreetPrefixMatch("прое"), ());
//  TEST(TestStreetPrefixMatch("проез"), ());
//  TEST(TestStreetPrefixMatch("проезд"), ());
//  TEST(!TestStreetPrefixMatch("проездд"), ());

//  TEST(TestStreetPrefixMatchWithMisprints("пр"), ());
//  TEST(!TestStreetPrefixMatch("пре"), ());
//  TEST(!TestStreetPrefixMatchWithMisprints("пре"), ());
//  TEST(!TestStreetPrefixMatch("преу"), ());
//  TEST(TestStreetPrefixMatchWithMisprints("преу"), ());
//  TEST(!TestStreetPrefixMatch("преул"), ());
//  TEST(TestStreetPrefixMatchWithMisprints("преул"), ());
//  TEST(!TestStreetPrefixMatch("преуло"), ());
//  TEST(TestStreetPrefixMatchWithMisprints("преуло"), ());
//  TEST(!TestStreetPrefixMatch("преулок"), ());
//  TEST(TestStreetPrefixMatchWithMisprints("преулок"), ());
//  TEST(!TestStreetPrefixMatch("преулак"), ());
//  TEST(!TestStreetPrefixMatchWithMisprints("преулак"), ());
}

UNIT_TEST(Street_TokensFilter)
{
  using List = vector<pair<string, size_t>>;

  {
    List expected = {};
    List actual;

    Utf8StreetTokensFilter filter(actual);
    filter.Put("ули", true /* isPrefix */, 0 /* tag */);

    TEST_EQUAL(expected, actual, ());
  }

  {
    List expected = {};
    List actual;

    Utf8StreetTokensFilter filter(actual);
    filter.Put("улица", false /* isPrefix */, 0 /* tag */);

    TEST_EQUAL(expected, actual, ());
  }

  {
    List expected = {{"генерала", 1}, {"антонова", 2}};
    List actual;

    Utf8StreetTokensFilter filter(actual);
    filter.Put("ул", false /* isPrefix */, 0 /* tag */);
    filter.Put("генерала", false /* isPrefix */, 1 /* tag */);
    filter.Put("антонова", false /* isPrefix */, 2 /* tag */);

    TEST_EQUAL(expected, actual, ());
  }

  {
    List expected = {{"набережная", 50}};
    List actual;

    Utf8StreetTokensFilter filter(actual);
    filter.Put("улица", false /* isPrefix */, 100 /* tag */);
    filter.Put("набережная", true /* isPrefix */, 50 /* tag */);

    TEST_EQUAL(expected, actual, ());
  }

  {
    List expected = {{"набережная", 1}, {"проспект", 2}};
    List actual;

    Utf8StreetTokensFilter filter(actual);
    filter.Put("улица", false /* isPrefix */, 0 /* tag */);
    filter.Put("набережная", true /* isPrefix */, 1 /* tag */);
    filter.Put("проспект", false /* isPrefix */, 2 /* tag */);

    TEST_EQUAL(expected, actual, ());
  }

  {
    List expected = {{"ленинский", 0}, {"пропект", 1}};
    List actualWithMisprints;
    List actualWithoutMisprints;

    Utf8StreetTokensFilter filterWithMisprints(actualWithMisprints, true /* withMisprints */);
    Utf8StreetTokensFilter filterWithoutMisprints(actualWithoutMisprints,
                                                  false /* withMisprints */);
    filterWithMisprints.Put("ленинский", false /* isPrefix */, 0 /* tag */);
    filterWithoutMisprints.Put("ленинский", false /* isPrefix */, 0 /* tag */);
    filterWithMisprints.Put("пропект", false /* isPrefix */, 1 /* tag */);
    filterWithoutMisprints.Put("пропект", false /* isPrefix */, 1 /* tag */);

    TEST_EQUAL(expected, actualWithMisprints, ());
    TEST_EQUAL(expected, actualWithoutMisprints, ());
  }

  {
    List expected = {{"набрежная", 1}};
    List actualWithMisprints;
    List actualWithoutMisprints;

    Utf8StreetTokensFilter filterWithMisprints(actualWithMisprints, true /* withMisprints */);
    Utf8StreetTokensFilter filterWithoutMisprints(actualWithoutMisprints,
                                                  false /* withMisprints */);
    filterWithMisprints.Put("улица", false /* isPrefix */, 0 /* tag */);
    filterWithoutMisprints.Put("улица", false /* isPrefix */, 0 /* tag */);
    filterWithMisprints.Put("набрежная", false /* isPrefix */, 1 /* tag */);
    filterWithoutMisprints.Put("набрежная", false /* isPrefix */, 1 /* tag */);

    TEST_EQUAL(expected, actualWithMisprints, ());
    TEST_EQUAL(expected, actualWithoutMisprints, ());
  }
}

UNIT_TEST(NormalizeAndSimplifyString_Numero)
{
  TEST_EQUAL(NormalizeAndSimplifyStringUtf8("Зона №51"), "зона #51", ());
  TEST_EQUAL(NormalizeAndSimplifyStringUtf8("Area № 51"), "area # 51", ());
  TEST_EQUAL(NormalizeAndSimplifyStringUtf8("Area #One"), "area #one", ());
}

} // namespace search_string_utils_test
