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
  saver.SaveNewQuery(record1);
  vector<string> const & result = saver.GetTopQueries();
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0], record1, ());
  saver.Clear();
}

UNIT_TEST(QuerySaverClearTest)
{
  QuerySaver saver;
  saver.SaveNewQuery(record1);
  TEST_GREATER(saver.GetTopQueries().size(), 0, ());
  saver.Clear();
  TEST_EQUAL(saver.GetTopQueries().size(), 0, ());
}

UNIT_TEST(QuerySaverOrderingTest)
{
  QuerySaver saver;
  saver.SaveNewQuery(record1);
  saver.SaveNewQuery(record2);
  {
    vector<string> const & result = saver.GetTopQueries();
    TEST_EQUAL(result.size(), 2, ());
    TEST_EQUAL(result[0], record1, ());
    TEST_EQUAL(result[1], record2, ());
  }
  saver.SaveNewQuery(record1);
  {
    vector<string> const & result = saver.GetTopQueries();
    TEST_EQUAL(result.size(), 2, ());
    TEST_EQUAL(result[1], record1, ());
    TEST_EQUAL(result[0], record2, ());
  }
  saver.Clear();
}

UNIT_TEST(QuerySaverSerializerTest)
{
  QuerySaver saver;
  saver.SaveNewQuery(record1);
  saver.SaveNewQuery(record2);
  vector<char> data;
  saver.Serialize(data);
  TEST_GREATER(data.size(), 0, ());
  saver.Clear();
  TEST_EQUAL(saver.GetTopQueries().size(), 0, ());
  saver.Deserialize(string(data.begin(), data.end()));

  vector<string> const & result = saver.GetTopQueries();
  TEST_EQUAL(result.size(), 2, ());
  TEST_EQUAL(result[0], record1, ());
  TEST_EQUAL(result[1], record2, ());
}

UNIT_TEST(QuerySaverPersistanceStore)
{
  {
    QuerySaver saver;
    saver.SaveNewQuery(record1);
    saver.SaveNewQuery(record2);
  }
  {
    QuerySaver saver;
    vector<string> const & result = saver.GetTopQueries();
    TEST_EQUAL(result.size(), 2, ());
    TEST_EQUAL(result[0], record1, ());
    TEST_EQUAL(result[1], record2, ());
    saver.Clear();
  }
}
}  // namespace search
