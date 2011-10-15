#include "../../testing/testing.hpp"

#include "../categories_holder.hpp"

#include "../../indexer/classificator.hpp"
#include "../../indexer/classificator_loader.hpp"

#include "../../platform/platform.hpp"

#include "../../coding/multilang_utf8_string.hpp"

#include "../../std/sstream.hpp"

char const * TEST_STRING =  "amenity-bench\n"
                            "en:1bench|sit down|to sit\n"
                            "de:0bank|auf die strafbank schicken\n"
                            "\n"
                            "place-village|place-hamlet\n"
                            "en:village\n"
                            "de:2dorf|4weiler";

struct Checker
{
  size_t & m_count;
  Checker(size_t & count) : m_count(count) {}
  void operator()(search::Category const & cat)
  {
    switch (m_count)
    {
    case 0:
      {
        TEST_EQUAL(cat.m_types.size(), 1, ());
        TEST_EQUAL(cat.m_synonyms.size(), 5, ());
        TEST_EQUAL(cat.m_synonyms[0].m_lang, StringUtf8Multilang::GetLangIndex("en"), ());
        TEST_EQUAL(cat.m_synonyms[0].m_name, "bench", ());
        TEST_EQUAL(cat.m_synonyms[0].m_prefixLengthToSuggest, 1, ());
        TEST_EQUAL(cat.m_synonyms[1].m_lang, StringUtf8Multilang::GetLangIndex("en"), ());
        TEST_EQUAL(cat.m_synonyms[1].m_name, "sit down", ());
        TEST_EQUAL(cat.m_synonyms[1].m_prefixLengthToSuggest, 10, ());
        TEST_EQUAL(cat.m_synonyms[2].m_lang, StringUtf8Multilang::GetLangIndex("en"), ());
        TEST_EQUAL(cat.m_synonyms[2].m_name, "to sit", ());
        TEST_EQUAL(cat.m_synonyms[3].m_lang, StringUtf8Multilang::GetLangIndex("de"), ());
        TEST_EQUAL(cat.m_synonyms[3].m_name, "bank", ());
        TEST_EQUAL(cat.m_synonyms[3].m_prefixLengthToSuggest, 0, ());
        TEST_EQUAL(cat.m_synonyms[4].m_lang, StringUtf8Multilang::GetLangIndex("de"), ());
        TEST_EQUAL(cat.m_synonyms[4].m_name, "auf die strafbank schicken", ());
        ++m_count;
      }
      break;
    case 1:
      {
        TEST_EQUAL(cat.m_types.size(), 2, ());
        TEST_EQUAL(cat.m_synonyms.size(), 3, ());
        TEST_EQUAL(cat.m_synonyms[0].m_lang, StringUtf8Multilang::GetLangIndex("en"), ());
        TEST_EQUAL(cat.m_synonyms[0].m_name, "village", ());
        TEST_EQUAL(cat.m_synonyms[1].m_lang, StringUtf8Multilang::GetLangIndex("de"), ());
        TEST_EQUAL(cat.m_synonyms[1].m_name, "dorf", ());
        TEST_EQUAL(cat.m_synonyms[1].m_prefixLengthToSuggest, 2, ());
        TEST_EQUAL(cat.m_synonyms[2].m_lang, StringUtf8Multilang::GetLangIndex("de"), ());
        TEST_EQUAL(cat.m_synonyms[2].m_name, "weiler", ());
        TEST_EQUAL(cat.m_synonyms[2].m_prefixLengthToSuggest, 4, ());
        ++m_count;
      }
      break;
    default: TEST(false, ("Too many categories"));
    }
  }
};

UNIT_TEST(LoadCategories)
{
  Platform & p = GetPlatform();
  classificator::Read(p.GetReader("drawing_rules.bin"),
                      p.GetReader("classificator.txt"),
                      p.GetReader("visibility.txt"),
                      p.GetReader("types.txt"));
  /*
  search::CategoriesHolder h;
  string buffer = TEST_STRING;
  TEST_GREATER(h.LoadFromStream(buffer), 0, ());
  size_t count = 0;
  Checker f(count);
  h.ForEachCategory(f);
  TEST_EQUAL(count, 2, ());
  */
}

