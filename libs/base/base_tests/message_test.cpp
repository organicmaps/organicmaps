#include "testing/testing.hpp"

UNIT_TEST(DebugPrint_Char)
{
  char const * s = "Тест";

  char const arr1[] = "Тест";
  char constexpr arr2[] = "Тест";
  char arr3[] = "Тест";

  std::string str(s);
  char * s2 = str.data();

  TEST_EQUAL(s, DebugPrint(s), ());
  TEST_EQUAL(s, DebugPrint(arr1), ());
  TEST_EQUAL(s, DebugPrint(arr2), ());
  TEST_EQUAL(s, DebugPrint(arr3), ());
  TEST_EQUAL(s, DebugPrint(s2), ());
  TEST_EQUAL(s, DebugPrint(str), ());
}
