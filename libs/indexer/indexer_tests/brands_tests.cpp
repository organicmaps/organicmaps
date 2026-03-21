#include "testing/testing.hpp"

#include "indexer/brands_holder.hpp"

#include "coding/reader.hpp"
#include "coding/string_utf8_multilang.hpp"

#include <memory>
#include <set>
#include <string>

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
  auto const & brands = indexer::GetDefaultBrands();

  TEST(!brands.GetKeys().empty(), ());
}

UNIT_TEST(LoadBrands)
{
  indexer::BrandsHolder const holder(std::make_unique<MemReader>(g_testBrandsTxt, sizeof(g_testBrandsTxt) - 1));

  std::set<std::string> expectedKeys = {"mcdonalds", "subway"};
  auto const keys = holder.GetKeys();
  TEST_EQUAL(keys, expectedKeys, ());

  using Names = std::set<indexer::BrandsHolder::Brand::Name>;

  {
    Names expectedNames;
    expectedNames.emplace("McDonald's", StringUtf8Multilang::GetLangIndex("en"));
    expectedNames.emplace("Mc Donalds", StringUtf8Multilang::GetLangIndex("en"));
    expectedNames.emplace("МакДональд'с", StringUtf8Multilang::GetLangIndex("ru"));
    expectedNames.emplace("Мак Доналдс", StringUtf8Multilang::GetLangIndex("ru"));
    expectedNames.emplace("Макдональдз", StringUtf8Multilang::GetLangIndex("uk"));

    Names names;
    holder.ForEachNameByKey("mcdonalds",
                            [&names](indexer::BrandsHolder::Brand::Name const & name) { names.insert(name); });
    CHECK_EQUAL(names, expectedNames, ());
  }

  {
    Names expectedNames;
    expectedNames.emplace("Subway", StringUtf8Multilang::GetLangIndex("en"));
    expectedNames.emplace("Сабвэй", StringUtf8Multilang::GetLangIndex("ru"));
    expectedNames.emplace("Сабвей", StringUtf8Multilang::GetLangIndex("ru"));

    Names names;
    holder.ForEachNameByKey("subway",
                            [&names](indexer::BrandsHolder::Brand::Name const & name) { names.insert(name); });
    CHECK_EQUAL(names, expectedNames, ());
  }

  {
    std::set<std::string> expectedNames = {"McDonald's", "Mc Donalds"};

    std::set<std::string> names;
    holder.ForEachNameByKeyAndLang("mcdonalds", "en", [&names](std::string const & name) { names.insert(name); });
    CHECK_EQUAL(names, expectedNames, ());
  }

  {
    std::set<std::string> expectedNames = {"МакДональд'с", "Мак Доналдс"};

    std::set<std::string> names;
    holder.ForEachNameByKeyAndLang("mcdonalds", "ru", [&names](std::string const & name) { names.insert(name); });
    CHECK_EQUAL(names, expectedNames, ());
  }

  {
    std::set<std::string> expectedNames = {"Макдональдз"};

    std::set<std::string> names;
    holder.ForEachNameByKeyAndLang("mcdonalds", "uk", [&names](std::string const & name) { names.insert(name); });
    CHECK_EQUAL(names, expectedNames, ());
  }
}
