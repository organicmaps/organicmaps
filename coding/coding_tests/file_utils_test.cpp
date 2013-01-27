#include "../../testing/testing.hpp"

#include "../file_name_utils.hpp"


UNIT_TEST(FileName_Smoke)
{
  string name = "/Users/xxx/Documents/test.test";
  my::GetNameFromFullPath(name);
  TEST_EQUAL(name, "test.test", ());
  my::GetNameFromFullPath(name);
  TEST_EQUAL(name, "test.test", ());
  my::GetNameWithoutExt(name);
  TEST_EQUAL(name, "test", ());

  name = "C:\\My Documents\\test.test";
  my::GetNameFromFullPath(name);
  TEST_EQUAL(name, "test.test", ());
  my::GetNameWithoutExt(name);
  TEST_EQUAL(name, "test", ());

  name = "/";
  my::GetNameFromFullPath(name);
  TEST(name.empty(), ());

  name = "C:\\";
  my::GetNameFromFullPath(name);
  TEST(name.empty(), ());

  name = "../test";
  my::GetNameFromFullPath(name);
  TEST_EQUAL(name, "test", ());
  my::GetNameWithoutExt(name);
  TEST_EQUAL(name, "test", ());
}
