#include "../../testing/testing.hpp"

#include "../categories_holder.hpp"

#include "../../indexer/classificator.hpp"
#include "../../indexer/classificator_loader.hpp"

#include "../../platform/platform.hpp"

#include "../../coding/multilang_utf8_string.hpp"

#include "../../std/sstream.hpp"

char const * TEST_STRING =  "amenity-bench\n"
                            "en:bench|sit down|to sit\n"
                            "de:bank|auf die strafbank schicken\n"
                            "\n"
                            "place-village|place-hamlet\n"
                            "en:village\n"
                            "de:dorf|weiler";

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
        TEST_EQUAL(cat.m_synonyms[0].first, StringUtf8Multilang::GetLangIndex("en"), ());
        TEST_EQUAL(cat.m_synonyms[0].second, "bench", ());
        TEST_EQUAL(cat.m_synonyms[1].first, StringUtf8Multilang::GetLangIndex("en"), ());
        TEST_EQUAL(cat.m_synonyms[1].second, "sit down", ());
        TEST_EQUAL(cat.m_synonyms[2].first, StringUtf8Multilang::GetLangIndex("en"), ());
        TEST_EQUAL(cat.m_synonyms[2].second, "to sit", ());
        TEST_EQUAL(cat.m_synonyms[3].first, StringUtf8Multilang::GetLangIndex("de"), ());
        TEST_EQUAL(cat.m_synonyms[3].second, "bank", ());
        TEST_EQUAL(cat.m_synonyms[4].first, StringUtf8Multilang::GetLangIndex("de"), ());
        TEST_EQUAL(cat.m_synonyms[4].second, "auf die strafbank schicken", ());
        ++m_count;
      }
      break;
    case 1:
      {
        TEST_EQUAL(cat.m_types.size(), 2, ());
        TEST_EQUAL(cat.m_synonyms.size(), 3, ());
        TEST_EQUAL(cat.m_synonyms[0].first, StringUtf8Multilang::GetLangIndex("en"), ());
        TEST_EQUAL(cat.m_synonyms[0].second, "village", ());
        TEST_EQUAL(cat.m_synonyms[1].first, StringUtf8Multilang::GetLangIndex("de"), ());
        TEST_EQUAL(cat.m_synonyms[1].second, "dorf", ());
        TEST_EQUAL(cat.m_synonyms[2].first, StringUtf8Multilang::GetLangIndex("de"), ());
        TEST_EQUAL(cat.m_synonyms[2].second, "weiler", ());
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
  classificator::Read(p.ReadPathForFile("drawing_rules.bin"),
                      p.ReadPathForFile("classificator.txt"),
                      p.ReadPathForFile("visibility.txt"));

  search::CategoriesHolder h;
  istringstream file(TEST_STRING);
  TEST_GREATER(h.LoadFromStream(file), 0, ());
  size_t count = 0;
  Checker f(count);
  h.ForEachCategory(f);
  TEST_EQUAL(count, 2, ());
}
