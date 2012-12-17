#include "../../testing/testing.hpp"

#include "../storage.hpp"


using namespace storage;


UNIT_TEST(StorageTest_Smoke)
{
  Storage st;

  TIndex const i1 = st.FindIndexByFile("USA_Georgia");
  TEST(i1.IsValid(), ());

  TIndex const i2 = st.FindIndexByFile("Georgia");
  TEST(i2.IsValid(), ());

  TEST_NOT_EQUAL(i1, i2, ());
}
