#include "testing/testing.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/categories_index.hpp"
#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/new_feature_categories.hpp"

#include "editor/editor_config.hpp"

#include "coding/multilang_utf8_string.hpp"
#include "coding/reader.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/sstream.hpp"
#include "std/vector.hpp"
#include "std/transform_iterator.hpp"

#include "base/stl_helpers.hpp"

using namespace indexer;

char const g_testCategoriesTxt[] =
    "amenity-bench\n"
    "en:1bench|sit down|to sit\n"
    "de:0bank|auf die strafbank schicken\n"
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
        TEST_EQUAL(cat.m_synonyms[3].m_prefixLengthToSuggest, 0, ());
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
  TEST_LESS(index.GetNumTrieNodes(), 250000, ());
}
#endif

#ifdef DEBUG
// A check that this data structure is not too heavy.
UNIT_TEST(CategoriesIndex_AllCategoriesEnglishName)
{
  classificator::Load();

  CategoriesIndex index;

  index.AddAllCategoriesInLang(CategoriesHolder::MapLocaleToInteger("en"));
  TEST_LESS(index.GetNumTrieNodes(), 6000, ());
}
#endif

UNIT_TEST(CategoriesIndex_UniqueNames)
{
  classificator::Load();
  auto const & cl = classif();

  editor::EditorConfig config;
  osm::NewFeatureCategories categories(config);

  for (auto const & lang : {"en", "ru", "de", "cs", "da", "es", "fi", "fr", "hu",
                            "id", "it", "ja", "co", "nl", "nb", "pl", "pt", "ro",
                            "sk", "sv", "th", "tr", "uk", "vi", "zh-Hant" "ar"})
  {
    categories.AddLanguage(lang);
    auto const & names = categories.GetAllCategoryNames(lang);

    auto firstFn = bind(&pair<string, uint32_t>::first, _1);
    set<string> uniqueNames(make_transform_iterator(names.begin(), firstFn),
                            make_transform_iterator(names.end(), firstFn));

    if (uniqueNames.size() != names.size())
    {
      LOG(LWARNING, ("Invalid category translations", lang));

      for (size_t i = 1; i < names.size(); ++i)
      {
        if (names[i-1].first == names[i].first)
        {
          LOG(LWARNING, (names[i].first,
                         cl.GetReadableObjectName(names[i].second),
                         cl.GetReadableObjectName(names[i-1].second)));
        }
      }

      LOG(LWARNING, ("+++++++++++++++++++++++++++++++++++++"));
    }
  }
}
