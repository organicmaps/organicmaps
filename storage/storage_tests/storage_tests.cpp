#include "testing/testing.hpp"

#include "storage/storage.hpp"

#include "defines.hpp"


using namespace storage;


UNIT_TEST(StorageTest_Smoke)
{
  Storage st;

  TIndex const i1 = st.FindIndexByFile("USA_Georgia");
  TEST(i1.IsValid(), ());
  TEST_EQUAL(st.CountryFileName(i1, TMapOptions::EMap), "USA_Georgia" DATA_FILE_EXTENSION, ());

  TIndex const i2 = st.FindIndexByFile("Georgia");
  TEST(i2.IsValid(), ());
  TEST_EQUAL(st.CountryFileName(i2, TMapOptions::ECarRouting), "Georgia" DATA_FILE_EXTENSION ROUTING_FILE_EXTENSION, ());

  TEST_NOT_EQUAL(i1, i2, ());
}
