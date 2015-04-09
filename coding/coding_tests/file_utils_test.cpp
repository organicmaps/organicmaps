#include "testing/testing.hpp"

#include "coding/file_name_utils.hpp"


UNIT_TEST(FileName_Smoke)
{
  string name = "/Users/xxx/Documents/test.test";
  TEST_EQUAL(my::GetFileExtension(name), ".test", ());
  my::GetNameFromFullPath(name);
  TEST_EQUAL(name, "test.test", ());
  my::GetNameFromFullPath(name);
  TEST_EQUAL(name, "test.test", ());
  my::GetNameWithoutExt(name);
  TEST_EQUAL(name, "test", ());

  name = "C:\\My Documents\\test.test";
  TEST_EQUAL(my::GetFileExtension(name), ".test", ());
  my::GetNameFromFullPath(name);
  TEST_EQUAL(name, "test.test", ());
  my::GetNameWithoutExt(name);
  TEST_EQUAL(name, "test", ());

  name = "/";
  TEST_EQUAL(my::GetFileExtension(name), string(), ());
  my::GetNameFromFullPath(name);
  TEST(name.empty(), ());

  name = "C:\\";
  TEST_EQUAL(my::GetFileExtension(name), string(), ());
  my::GetNameFromFullPath(name);
  TEST(name.empty(), ());

  name = "../test";
  TEST_EQUAL(my::GetFileExtension(name), string(), ());
  my::GetNameFromFullPath(name);
  TEST_EQUAL(name, "test", ());
  my::GetNameWithoutExt(name);
  TEST_EQUAL(name, "test", ());
}
