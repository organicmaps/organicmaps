#include "testing/testing.hpp"

#include "search/query_saver.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace
{
string const record1("test record1");
string const record2("sometext");
}

namespace search
{
UNIT_TEST(QuerySaverFogTest)
{
  QuerySaver saver;
  saver.Clear();
  saver.Add(record1);
  list<string> const & result = saver.Get();
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
    list<string> const & result = saver.Get();
    TEST_EQUAL(result.size(), 2, ());
    TEST_EQUAL(result.back(), record1, ());
    TEST_EQUAL(result.front(), record2, ());
  }
  saver.Add(record1);
  {
    list<string> const & result = saver.Get();
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
  string data;
  saver.Serialize(data);
  TEST_GREATER(data.size(), 0, ());
  saver.Clear();
  TEST_EQUAL(saver.Get().size(), 0, ());
  saver.Deserialize(data);

  list<string> const & result = saver.Get();
  TEST_EQUAL(result.size(), 2, ());
  TEST_EQUAL(result.back(), record1, ());
  TEST_EQUAL(result.front(), record2, ());
}

UNIT_TEST(QuerySaverCorruptedStringTest)
{
  QuerySaver saver;
  string corrupted("DEADBEEF");
  saver.Deserialize(corrupted);
  list<string> const & result = saver.Get();
  TEST_EQUAL(result.size(), 0, ());
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
    list<string> const & result = saver.Get();
    TEST_EQUAL(result.size(), 2, ());
    TEST_EQUAL(result.back(), record1, ());
    TEST_EQUAL(result.front(), record2, ());
    saver.Clear();
  }
}
}  // namespace search
