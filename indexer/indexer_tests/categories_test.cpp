#include "testing/testing.hpp"

#include "indexer/categories_holder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "std/sstream.hpp"


char const * TEST_STRING =  "amenity-bench\n"
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

  CategoriesHolder h;
  istringstream buffer(TEST_STRING);
  h.LoadFromStream(buffer);

  size_t count = 0;
  Checker f(count);
  h.ForEachCategory(f);
  TEST_EQUAL(count, 3, ());
}
