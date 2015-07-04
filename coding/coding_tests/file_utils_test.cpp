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

// TODO (@gorshenin): implement a Clean() method to clean file path
// (remove redundant separators, correctly collapse dots, dot-dots, etc.).
#ifndef OMIM_OS_WINDOWS

UNIT_TEST(FileName_GetDirectory)
{
  TEST_EQUAL("/tmp", my::GetDirectory("/tmp/evil\\file"), ());
  TEST_EQUAL(".", my::GetDirectory("evil\\file"), ());

  TEST_EQUAL("/", my::GetDirectory("/somefile.txt"), ());

  TEST_EQUAL("/", my::GetDirectory("////somefile"), ());
  TEST_EQUAL("a/b", my::GetDirectory("a/b///somefile.txt"), ());

  TEST_EQUAL("/a/b", my::GetDirectory("/a/b/c"), ());
  TEST_EQUAL("/a/b", my::GetDirectory("/a/b/c.txt"), ());

  TEST_EQUAL(".", my::GetDirectory("somefile.txt"), ());
  TEST_EQUAL(".", my::GetDirectory("somefile"), ());
}

UNIT_TEST(FilePath_Slash)
{
  TEST_EQUAL("/", my::AddSlashIfNeeded(""), ());
  TEST_EQUAL("/", my::AddSlashIfNeeded("/"), ());
  TEST_EQUAL("./", my::AddSlashIfNeeded("."), ());
  TEST_EQUAL("data/", my::AddSlashIfNeeded("data"), ());
  TEST_EQUAL("data/", my::AddSlashIfNeeded("data/"), ());
  TEST_EQUAL("/data/", my::AddSlashIfNeeded("/data"), ());
  TEST_EQUAL("/data/", my::AddSlashIfNeeded("/data/"), ());
  TEST_EQUAL("../../data/", my::AddSlashIfNeeded("../../data"), ());
  TEST_EQUAL("../../data/", my::AddSlashIfNeeded("../../data/"), ());
}

UNIT_TEST(FilePath_Join)
{
  TEST_EQUAL("omim/strings.txt", my::JoinFoldersToPath("omim", "strings.txt"), ());
  TEST_EQUAL("omim/strings.txt", my::JoinFoldersToPath("omim/", "strings.txt"), ());
  TEST_EQUAL("../../omim/strings.txt", my::JoinFoldersToPath({"..", "..", "omim"}, "strings.txt"), ());
  TEST_EQUAL("../../omim/strings.txt", my::JoinFoldersToPath({"../", "..", "omim/"}, "strings.txt"), ());
}

#endif // OMIM_OS_WINDOWS
