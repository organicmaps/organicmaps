#include "testing/testing.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/categories_index.hpp"
#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "coding/reader.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <memory>
#include <vector>

using namespace indexer;
using namespace std;

char const g_testCategoriesTxt[] =
    "amenity-bench\n"
    "en:1bench|sit down|to sit\n"
    "de:2bank|auf die strafbank schicken\n"
    "zh-Hans:长凳\n"
    "zh-Hant:長板凳\n"
    "da:bænk\n"
    "\n"
    "place-village|place-hamlet\n"
    "en:village\n"
    "de:2dorf|4weiler";

struct Checker
{
  size_t & m_count;
  Checker(size_t & count) : m_count(count) {}
  void operator()(CategoriesHolder::Category const & cat)
  {
    switch (m_count)
    {
    case 0:
      {
        TEST_EQUAL(cat.m_synonyms.size(), 8, ());
        TEST_EQUAL(cat.m_synonyms[0].m_locale, CategoriesHolder::MapLocaleToInteger("en"), ());
        TEST_EQUAL(cat.m_synonyms[0].m_name, "bench", ());
        TEST_EQUAL(cat.m_synonyms[0].m_prefixLengthToSuggest, 1, ());
        TEST_EQUAL(cat.m_synonyms[1].m_locale, CategoriesHolder::MapLocaleToInteger("en"), ());
        TEST_EQUAL(cat.m_synonyms[1].m_name, "sit down", ());
        TEST_EQUAL(cat.m_synonyms[1].m_prefixLengthToSuggest, 10, ());
        TEST_EQUAL(cat.m_synonyms[2].m_locale, CategoriesHolder::MapLocaleToInteger("en"), ());
        TEST_EQUAL(cat.m_synonyms[2].m_name, "to sit", ());
        TEST_EQUAL(cat.m_synonyms[3].m_locale, CategoriesHolder::MapLocaleToInteger("de"), ());
        TEST_EQUAL(cat.m_synonyms[3].m_name, "bank", ());
        TEST_EQUAL(cat.m_synonyms[3].m_prefixLengthToSuggest, 2, ());
        TEST_EQUAL(cat.m_synonyms[4].m_locale, CategoriesHolder::MapLocaleToInteger("de"), ());
        TEST_EQUAL(cat.m_synonyms[4].m_name, "auf die strafbank schicken", ());
        TEST_EQUAL(cat.m_synonyms[5].m_locale, CategoriesHolder::MapLocaleToInteger("zh_CN"), ());
        TEST_EQUAL(cat.m_synonyms[5].m_locale, CategoriesHolder::MapLocaleToInteger("zh_rCN"), ());
        TEST_EQUAL(cat.m_synonyms[5].m_locale, CategoriesHolder::MapLocaleToInteger("zh_HANS_CN"), ());
        TEST_EQUAL(cat.m_synonyms[5].m_locale, CategoriesHolder::MapLocaleToInteger("zh-Hans"), ());
        TEST_EQUAL(cat.m_synonyms[5].m_name, "长凳", ());
        TEST_EQUAL(cat.m_synonyms[6].m_locale, CategoriesHolder::MapLocaleToInteger("zh_TW"), ());
        TEST_EQUAL(cat.m_synonyms[6].m_locale, CategoriesHolder::MapLocaleToInteger("zh-MO"), ());
        TEST_EQUAL(cat.m_synonyms[6].m_locale, CategoriesHolder::MapLocaleToInteger("zh-rTW"), ());
        TEST_EQUAL(cat.m_synonyms[6].m_locale, CategoriesHolder::MapLocaleToInteger("zh_HANT_HK"), ());
        TEST_EQUAL(cat.m_synonyms[6].m_locale, CategoriesHolder::MapLocaleToInteger("zh_HK"), ());
        TEST_EQUAL(cat.m_synonyms[6].m_locale, CategoriesHolder::MapLocaleToInteger("zh-Hant"), ());
        TEST_EQUAL(cat.m_synonyms[6].m_name, "長板凳", ());
        TEST_EQUAL(cat.m_synonyms[7].m_locale, CategoriesHolder::MapLocaleToInteger("da"), ());
        TEST_EQUAL(cat.m_synonyms[7].m_name, "bænk", ());
        ++m_count;
      }
      break;
    case 1:
    case 2:
      {
        TEST_EQUAL(cat.m_synonyms.size(), 3, ());
        TEST_EQUAL(cat.m_synonyms[0].m_locale, CategoriesHolder::MapLocaleToInteger("en"), ());
        TEST_EQUAL(cat.m_synonyms[0].m_name, "village", ());
        TEST_EQUAL(cat.m_synonyms[1].m_locale, CategoriesHolder::MapLocaleToInteger("de"), ());
        TEST_EQUAL(cat.m_synonyms[1].m_name, "dorf", ());
        TEST_EQUAL(cat.m_synonyms[1].m_prefixLengthToSuggest, 2, ());
        TEST_EQUAL(cat.m_synonyms[2].m_locale, CategoriesHolder::MapLocaleToInteger("de"), ());
        TEST_EQUAL(cat.m_synonyms[2].m_name, "weiler", ());
        TEST_EQUAL(cat.m_synonyms[2].m_prefixLengthToSuggest, 4, ());
        ++m_count;
      }
      break;
    default:
      TEST(false, ("Too many categories"));
    }
  }
};

UNIT_TEST(LoadCategories)
{
  classificator::Load();

  CategoriesHolder h(make_unique<MemReader>(g_testCategoriesTxt, sizeof(g_testCategoriesTxt) - 1));
  size_t count = 0;
  Checker f(count);
  h.ForEachCategory(f);
  TEST_EQUAL(count, 3, ());
}

UNIT_TEST(CategoriesHolder_Smoke)
{
  auto const & mappings = CategoriesHolder::kLocaleMapping;
  for (size_t i = 0; i < mappings.size(); ++i)
  {
    auto const & mapping  = mappings[i];
    TEST_EQUAL(static_cast<int8_t>(i + 1), mapping.m_code, ());
    TEST_EQUAL(static_cast<int8_t>(i + 1),
               CategoriesHolder::MapLocaleToInteger(mapping.m_name), ());
    TEST_EQUAL(CategoriesHolder::MapIntegerToLocale(i + 1), mapping.m_name, ());
  }
}

UNIT_TEST(CategoriesHolder_DisplayedNameSmoke)
{
  classificator::Load();

  auto const & categoriesHolder = GetDefaultCategories();
  auto const & groupTranslations = categoriesHolder.GetGroupTranslations();

  categoriesHolder.ForEachCategory([](CategoriesHolder::Category const & cat) {
    for (auto const & synonym : cat.m_synonyms)
    {
      TEST_NOT_EQUAL(synonym.m_name[0], '^', ("symbol ^ is used incorrectly in categories.txt "
                                              "and loaded to synonyms."));
    }
  });

  for (auto const & group : groupTranslations)
  {
    for (auto const & translation : group.second)
    {
      TEST_NOT_EQUAL(translation.m_name[0], '^', ("symbol ^ is used incorrectly in categories.txt "
                                                  "and loaded to group translations"));
    }
  }
}

UNIT_TEST(CategoriesHolder_LoadDefault)
{
  classificator::Load();

  uint32_t counter = 0;
  auto const count = [&counter](CategoriesHolder::Category const &) { ++counter; };

  auto const & categoriesHolder = GetDefaultCategories();
  categoriesHolder.ForEachCategory(count);
  TEST_GREATER(counter, 0, ());

  counter = 0;
  auto const & cuisineCategoriesHolder = GetDefaultCuisineCategories();
  cuisineCategoriesHolder.ForEachCategory(count);
  TEST_GREATER(counter, 0, ());
}

UNIT_TEST(CategoriesHolder_DisplayedName)
{
  char const kCategories[] =
      "@shop\n"
      "en:^Shop\n"
      "ru:^Mагазин\n"
      "\n"
      "@meat\n"
      "en:Beef|^Meat\n"
      "ru:мясо\n"
      "de:Schlachter\n"
      "\n"
      "@butcher\n"
      "de:^Metzgerei\n"
      "\n"
      "shop|@shop\n"
      "en:market\n"
      "\n"
      "shop-alcohol|@shop\n"
      "en:Liquor Store|2^Alcostore\n"
      "\n"
      "shop-bakery|@shop\n"
      "en:^buns\n"
      "\n"
      "shop-butcher|@meat|@butcher\n"
      "en:2butcher\n"
      "ru:3^Мясная лавка\n"
      "de:Geschäft|2Laden\n"
      "";

  classificator::Load();
  CategoriesHolder holder(make_unique<MemReader>(kCategories, ARRAY_SIZE(kCategories) - 1));

  holder.ForEachTypeAndCategory([](uint32_t const type, CategoriesHolder::Category const & cat) {
    auto const readableTypeName = classif().GetReadableObjectName(type);
    if (readableTypeName == "shop")
    {
      TEST_EQUAL(cat.m_synonyms.size(), 3, ());
      TEST_EQUAL(cat.m_synonyms[0].m_name, "Mагазин", ());
      TEST_EQUAL(cat.m_synonyms[1].m_name, "Shop", ());
      TEST_EQUAL(cat.m_synonyms[2].m_name, "market", ());
    }
    else if (readableTypeName == "shop-alcohol")
    {
      TEST_EQUAL(cat.m_synonyms.size(), 4, ());
      TEST_EQUAL(cat.m_synonyms[0].m_name, "Alcostore", ());
      TEST_EQUAL(cat.m_synonyms[1].m_name, "Mагазин", ());
      TEST_EQUAL(cat.m_synonyms[2].m_name, "Shop", ());
      TEST_EQUAL(cat.m_synonyms[3].m_name, "Liquor Store", ());
    }
    else if (readableTypeName == "shop-bakery")
    {
      TEST_EQUAL(cat.m_synonyms.size(), 3, ());
      TEST_EQUAL(cat.m_synonyms[0].m_name, "buns", ());
      TEST_EQUAL(cat.m_synonyms[1].m_name, "Mагазин", ());
      TEST_EQUAL(cat.m_synonyms[2].m_name, "Shop", ());
    }
    else if (readableTypeName == "shop-butcher")
    {
      TEST_EQUAL(cat.m_synonyms.size(), 9, ());
      TEST_EQUAL(cat.m_synonyms[0].m_name, "Мясная лавка", ());
      TEST_EQUAL(cat.m_synonyms[1].m_name, "Metzgerei", ());
      TEST_EQUAL(cat.m_synonyms[2].m_name, "Meat", ());
      TEST_EQUAL(cat.m_synonyms[3].m_name, "Beef", ());
      TEST_EQUAL(cat.m_synonyms[4].m_name, "мясо", ());
      TEST_EQUAL(cat.m_synonyms[5].m_name, "Schlachter", ());
      TEST_EQUAL(cat.m_synonyms[6].m_name, "butcher", ());
      TEST_EQUAL(cat.m_synonyms[7].m_name, "Geschäft", ());
      TEST_EQUAL(cat.m_synonyms[8].m_name, "Laden", ());
    }
    else
    {
      TEST(false, ("Unexpected group name:", readableTypeName));
    }
  });
}

UNIT_TEST(CategoriesHolder_ForEach)
{
  char const kCategories[] =
      "amenity-bar\n"
      "en:abc|ddd-eee\n"
      "\n"
      "amenity-pub\n"
      "en:ddd\n"
      "\n"
      "amenity-cafe\n"
      "en:abc eee\n"
      "\n"
      "amenity-restaurant\n"
      "en:ddd|eee\n"
      "\n"
      "";

  classificator::Load();
  CategoriesHolder holder(make_unique<MemReader>(kCategories, ARRAY_SIZE(kCategories) - 1));

  {
    uint32_t counter = 0;
    holder.ForEachTypeByName(CategoriesHolder::kEnglishCode, strings::MakeUniString("abc"),
                             [&](uint32_t /* type */) { ++counter; });
    TEST_EQUAL(counter, 2, ());
  }

  {
    uint32_t counter = 0;
    holder.ForEachTypeByName(CategoriesHolder::kEnglishCode, strings::MakeUniString("ddd"),
                             [&](uint32_t /* type */) { ++counter; });
    TEST_EQUAL(counter, 3, ());
  }

  {
    uint32_t counter = 0;
    holder.ForEachTypeByName(CategoriesHolder::kEnglishCode, strings::MakeUniString("eee"),
                             [&](uint32_t /* type */) { ++counter; });
    TEST_EQUAL(counter, 3, ());
  }
}

UNIT_TEST(CategoriesIndex_Smoke)
{
  classificator::Load();

  CategoriesHolder holder(
      make_unique<MemReader>(g_testCategoriesTxt, sizeof(g_testCategoriesTxt) - 1));
  CategoriesIndex index(holder);

  uint32_t type1 = classif().GetTypeByPath({"amenity", "bench"});
  uint32_t type2 = classif().GetTypeByPath({"place", "village"});
  if (type1 > type2)
    swap(type1, type2);
  int8_t lang1 = CategoriesHolder::MapLocaleToInteger("en");
  int8_t lang2 = CategoriesHolder::MapLocaleToInteger("de");

  auto testTypes = [&](string const & query, vector<uint32_t> const & expected)
  {
    vector<uint32_t> result;
    index.GetAssociatedTypes(query, result);
    TEST_EQUAL(result, expected, (query));
  };

  index.AddCategoryByTypeAndLang(type1, lang1);
  testTypes("bench", {type1});
  testTypes("BENCH", {type1});
  testTypes("down", {type1});
  testTypes("benck", {});
  testTypes("strafbank", {});
  index.AddCategoryByTypeAndLang(type1, lang2);
  testTypes("strafbank", {type1});
  testTypes("ie strafbank sc", {type1});
  testTypes("rafb", {type1});
  index.AddCategoryByTypeAndLang(type2, lang1);
  testTypes("i", {type1, type2});

  CategoriesIndex fullIndex(holder);
  fullIndex.AddCategoryByTypeAllLangs(type1);
  fullIndex.AddCategoryByTypeAllLangs(type2);
  vector<CategoriesHolder::Category> cats;

  // The letter 'a' matches "strafbank" and "village".
  // One language is not enough.
  fullIndex.GetCategories("a", cats);

  TEST_EQUAL(cats.size(), 2, ());

  TEST_EQUAL(cats[0].m_synonyms.size(), 8, ());
  TEST_EQUAL(cats[0].m_synonyms[4].m_locale, CategoriesHolder::MapLocaleToInteger("de"), ());
  TEST_EQUAL(cats[0].m_synonyms[4].m_name, "auf die strafbank schicken", ());

  TEST_EQUAL(cats[1].m_synonyms.size(), 3, ());
  TEST_EQUAL(cats[1].m_synonyms[0].m_locale, CategoriesHolder::MapLocaleToInteger("en"), ());
  TEST_EQUAL(cats[1].m_synonyms[0].m_name, "village", ());
}

UNIT_TEST(CategoriesIndex_MultipleTokens)
{
  char const kCategories[] =
      "shop-bakery\n"
      "en:shop of buns\n"
      "\n"
      "shop-butcher\n"
      "en:shop of meat";

  classificator::Load();
  CategoriesHolder holder(make_unique<MemReader>(kCategories, sizeof(kCategories) - 1));
  CategoriesIndex index(holder);

  index.AddAllCategoriesInAllLangs();
  auto testTypes = [&](string const & query, vector<uint32_t> const & expected)
  {
    vector<uint32_t> result;
    index.GetAssociatedTypes(query, result);
    TEST_EQUAL(result, expected, (query));
  };

  uint32_t type1 = classif().GetTypeByPath({"shop", "bakery"});
  uint32_t type2 = classif().GetTypeByPath({"shop", "butcher"});
  if (type1 > type2)
    swap(type1, type2);

  testTypes("shop", {type1, type2});
  testTypes("shop buns", {type1});
  testTypes("shop meat", {type2});
}

UNIT_TEST(CategoriesIndex_Groups)
{
  char const kCategories[] =
      "@shop\n"
      "en:shop\n"
      "ru:магазин\n"
      "\n"
      "@meat\n"
      "en:meat\n"
      "\n"
      "shop-bakery|@shop\n"
      "en:buns\n"
      "\n"
      "shop-butcher|@shop|@meat\n"
      "en:butcher\n"
      "";

  classificator::Load();
  CategoriesHolder holder(make_unique<MemReader>(kCategories, sizeof(kCategories) - 1));
  CategoriesIndex index(holder);

  index.AddAllCategoriesInAllLangs();
  auto testTypes = [&](string const & query, vector<uint32_t> const & expected)
  {
    vector<uint32_t> result;
    index.GetAssociatedTypes(query, result);
    TEST_EQUAL(result, expected, (query));
  };

  uint32_t type1 = classif().GetTypeByPath({"shop", "bakery"});
  uint32_t type2 = classif().GetTypeByPath({"shop", "butcher"});
  if (type1 > type2)
    swap(type1, type2);

  testTypes("buns", {type1});
  testTypes("butcher", {type2});
  testTypes("meat", {type2});
  testTypes("shop", {type1, type2});
  testTypes("магазин", {type1, type2});
  testTypes("http", {});
}

#ifdef DEBUG
// A check that this data structure is not too heavy.
UNIT_TEST(CategoriesIndex_AllCategories)
{
  classificator::Load();

  CategoriesIndex index;

  index.AddAllCategoriesInAllLangs();
  // Consider deprecating this method if this bound rises as high as a million.
  LOG(LINFO, ("Number of nodes in the CategoriesIndex trie:", index.GetNumTrieNodes()));
  TEST_LESS(index.GetNumTrieNodes(), 400000, ());
}
#endif

#ifdef DEBUG
// A check that this data structure is not too heavy.
UNIT_TEST(CategoriesIndex_AllCategoriesEnglishName)
{
  classificator::Load();

  CategoriesIndex index;

  index.AddAllCategoriesInLang(CategoriesHolder::MapLocaleToInteger("en"));
  TEST_LESS(index.GetNumTrieNodes(), 8000, ());
}
#endif
