#include "testing/testing.hpp"

#include "indexer/custom_keyvalue.hpp"

UNIT_TEST(CustomKeyValue_Smoke)
{
  indexer::CustomKeyValue empty("");
  TEST(!empty.Get(0), ());
  TEST_EQUAL(empty.ToString(), "", ());

  indexer::CustomKeyValue kv;
  kv.Add(1, 777);
  kv.Add(255, 0xFFFFFFFF);
  kv.Add(0, 0);

  indexer::CustomKeyValue actual(kv.ToString());
  TEST(!actual.Get(5), ());
  TEST_EQUAL(actual.GetSure(1), 777, ());
  TEST_EQUAL(actual.GetSure(255), 0xFFFFFFFF, ());
  TEST_EQUAL(actual.GetSure(0), 0, ());
}
