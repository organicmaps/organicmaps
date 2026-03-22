#include "testing/testing.hpp"

#include "search/query_saver.hpp"

#include <list>

namespace search  // to make private QuerySaver methods accessible for tests
{
namespace
{
search::QuerySaver::SearchRequest const record1("RU_ru", "test record1");
search::QuerySaver::SearchRequest const record2("En_us", "sometext");
}  // namespace

UNIT_TEST(QuerySaverFogTest)
{
  QuerySaver saver;
  saver.Clear();
  saver.Add(record1);
  auto const & result = saver.Get();
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result.front(), record1, ());
  saver.Clear();
}

UNIT_TEST(QuerySaverClearTest)
{
  QuerySaver saver;
  saver.Clear();
  saver.Add(record1);
  TEST_GREATER(saver.Get().size(), 0, ());
  saver.Clear();
  TEST_EQUAL(saver.Get().size(), 0, ());
}

UNIT_TEST(QuerySaverOrderingTest)
{
  QuerySaver saver;
  saver.Clear();
  saver.Add(record1);
  saver.Add(record2);
  {
    auto const & result = saver.Get();
    TEST_EQUAL(result.size(), 2, ());
    TEST_EQUAL(result.back(), record1, ());
    TEST_EQUAL(result.front(), record2, ());
  }
  saver.Add(record1);
  {
    auto const & result = saver.Get();
    TEST_EQUAL(result.size(), 2, ());
    TEST_EQUAL(result.front(), record1, ());
    TEST_EQUAL(result.back(), record2, ());
  }
  saver.Clear();
}

UNIT_TEST(QuerySaverSerializerTest)
{
  QuerySaver saver;
  saver.Clear();
  saver.Add(record1);
  saver.Add(record2);
  std::string data;
  saver.Serialize(data);
  TEST_GREATER(data.size(), 0, ());
  saver.Clear();
  TEST_EQUAL(saver.Get().size(), 0, ());
  saver.Deserialize(data);

  auto const & result = saver.Get();
  TEST_EQUAL(result.size(), 2, ());
  TEST_EQUAL(result.back(), record1, ());
  TEST_EQUAL(result.front(), record2, ());
}

UNIT_TEST(QuerySaverCorruptedStringTest)
{
  QuerySaver saver;
  std::string corrupted("DEADBEEF");
  bool exceptionThrown = false;
  try
  {
    saver.Deserialize(corrupted);
  }
  catch (RootException const & /* exception */)
  {
    exceptionThrown = true;
  }
  auto const & result = saver.Get();
  TEST_EQUAL(result.size(), 0, ());
  TEST(exceptionThrown, ());
}

UNIT_TEST(QuerySaverPersistanceStore)
{
  {
    QuerySaver saver;
    saver.Clear();
    saver.Add(record1);
    saver.Add(record2);
  }
  {
    QuerySaver saver;
    auto const & result = saver.Get();
    TEST_EQUAL(result.size(), 2, ());
    TEST_EQUAL(result.back(), record1, ());
    TEST_EQUAL(result.front(), record2, ());
    saver.Clear();
  }
}

UNIT_TEST(QuerySaverTrimRequestTest)
{
  QuerySaver saver;
  saver.Clear();

  search::QuerySaver::SearchRequest const rec1("RU_ru", "test record1");
  search::QuerySaver::SearchRequest const rec2("RU_ru", "test record1 ");

  saver.Add(rec1);
  saver.Add(rec2);

  {
    auto const & result = saver.Get();
    TEST_EQUAL(result.size(), 1, ());
    TEST_EQUAL(result.front(), rec2, ());
  }
  saver.Clear();

  saver.Add(rec2);
  saver.Add(rec1);

  {
    auto const & result = saver.Get();
    TEST_EQUAL(result.size(), 1, ());
    TEST_EQUAL(result.front(), rec1, ());
  }
  saver.Clear();
}
}  // namespace search
