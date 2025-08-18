#include "testing/testing.hpp"

#include "indexer/brands_holder.hpp"

#include "coding/reader.hpp"
#include "coding/string_utf8_multilang.hpp"

#include <memory>
#include <set>
#include <string>

using namespace std;
using namespace indexer;

char const g_testBrandsTxt[] =
    "brand.mcdonalds\n"
    "en:McDonald's|Mc Donalds\n"
    "ru:МакДональд'с|Мак Доналдс\n"
    "uk:Макдональдз\n"
    "\n"
    "brand.subway\n"
    "en:Subway\n"
    "ru:Сабвэй|Сабвей";

UNIT_TEST(LoadDefaultBrands)
{
  auto const & brands = GetDefaultBrands();

  TEST(!brands.GetKeys().empty(), ());
}

UNIT_TEST(LoadBrands)
{
  BrandsHolder const holder(make_unique<MemReader>(g_testBrandsTxt, sizeof(g_testBrandsTxt) - 1));

  set<string> expectedKeys = {"mcdonalds", "subway"};
  auto const keys = holder.GetKeys();
  TEST_EQUAL(keys, expectedKeys, ());

  using Names = set<BrandsHolder::Brand::Name>;

  {
    Names expectedNames;
    expectedNames.emplace("McDonald's", StringUtf8Multilang::GetLangIndex("en"));
    expectedNames.emplace("Mc Donalds", StringUtf8Multilang::GetLangIndex("en"));
    expectedNames.emplace("МакДональд'с", StringUtf8Multilang::GetLangIndex("ru"));
    expectedNames.emplace("Мак Доналдс", StringUtf8Multilang::GetLangIndex("ru"));
    expectedNames.emplace("Макдональдз", StringUtf8Multilang::GetLangIndex("uk"));

    Names names;
    holder.ForEachNameByKey("mcdonalds", [&names](BrandsHolder::Brand::Name const & name) { names.insert(name); });
    CHECK_EQUAL(names, expectedNames, ());
  }

  {
    Names expectedNames;
    expectedNames.emplace("Subway", StringUtf8Multilang::GetLangIndex("en"));
    expectedNames.emplace("Сабвэй", StringUtf8Multilang::GetLangIndex("ru"));
    expectedNames.emplace("Сабвей", StringUtf8Multilang::GetLangIndex("ru"));

    Names names;
    holder.ForEachNameByKey("subway", [&names](BrandsHolder::Brand::Name const & name) { names.insert(name); });
    CHECK_EQUAL(names, expectedNames, ());
  }

  {
    set<string> expectedNames = {"McDonald's", "Mc Donalds"};

    set<string> names;
    holder.ForEachNameByKeyAndLang("mcdonalds", "en", [&names](string const & name) { names.insert(name); });
    CHECK_EQUAL(names, expectedNames, ());
  }

  {
    set<string> expectedNames = {"МакДональд'с", "Мак Доналдс"};

    set<string> names;
    holder.ForEachNameByKeyAndLang("mcdonalds", "ru", [&names](string const & name) { names.insert(name); });
    CHECK_EQUAL(names, expectedNames, ());
  }

  {
    set<string> expectedNames = {"Макдональдз"};

    set<string> names;
    holder.ForEachNameByKeyAndLang("mcdonalds", "uk", [&names](string const & name) { names.insert(name); });
    CHECK_EQUAL(names, expectedNames, ());
  }
}
