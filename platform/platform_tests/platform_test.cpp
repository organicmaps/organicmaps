#include "../../testing/testing.hpp"

#include "../platform.hpp"

#include "../../storage/defines.hpp"

#include "../../std/stdio.hpp"

#include "../../base/start_mem_debug.hpp"

UNIT_TEST(TimeInSec)
{
  Platform & pl = GetPlatform();

  double t1 = pl.TimeInSec();
  double s = 0.0;
  for (int i = 0; i < 10000000; ++i)
    s += i*0.01;
  double t2 = pl.TimeInSec();

  TEST_NOT_EQUAL(s, 0.0, ("Fictive, to prevent loop optimization"));
  TEST_NOT_EQUAL(t1, t2, ("Timer values should not be equal"));

#ifndef DEBUG
  t1 = pl.TimeInSec();
  for (int i = 0; i < 10000000; ++i) {}
  t2 = pl.TimeInSec();

  TEST_EQUAL(t1, t2, ("Timer values should be equal: compiler loop optimization!"));
#endif
}

char const * TEST_FILE_NAME = "some_temporary_unit_test_file.tmp";

UNIT_TEST(WritableDir)
{
  string const path = GetPlatform().WritableDir() + TEST_FILE_NAME;
  FILE * f = fopen(path.c_str(), "w");
  TEST_NOT_EQUAL(f, 0, ("Can't create file", path));
  if (f)
    fclose(f);
  remove(path.c_str());
}

UNIT_TEST(WritablePathForFile)
{
  string const p1 = GetPlatform().WritableDir() + TEST_FILE_NAME;
  string const p2 = GetPlatform().WritablePathForFile(TEST_FILE_NAME);
  TEST_EQUAL(p1, p2, ());
}

UNIT_TEST(ReadPathForFile)
{
  char const * NON_EXISTING_FILE = "mgbwuerhsnmbui45efhdbn34.tmp";
  char const * arr[] = { "drawing_rules.bin", "basic.skn", "classificator.txt", "minsk-pass.mwm" };
  Platform & p = GetPlatform();
  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    TEST_GREATER( p.ReadPathForFile(arr[i]).size(), 0, ("File should exist!") );
  }

  bool wasException = false;
  try
  {
    p.ReadPathForFile(NON_EXISTING_FILE);
  }
  catch (FileAbsentException const &)
  {
    wasException = true;
  }
  TEST( wasException, ());
}

UNIT_TEST(GetFilesInDir)
{
  Platform & pl = GetPlatform();
  Platform::FilesList files;
  TEST_GREATER(pl.GetFilesInDir(pl.WritableDir(), "*" DATA_FILE_EXTENSION, files), 0, ("/data/ folder should contain some data files"));

  TEST_EQUAL(pl.GetFilesInDir(pl.WritableDir(), "asdnonexistentfile.dsa", files), 0, ());
  TEST_EQUAL(files.size(), 0, ());
}

UNIT_TEST(GetFileSize)
{
  Platform & pl = GetPlatform();
  uint64_t size = 0;
  pl.GetFileSize(pl.ReadPathForFile("classificator.txt").c_str(), size);
  TEST_GREATER(size, 0, ("File classificator.txt should exist for test"));
}

UNIT_TEST(CpuCores)
{
  int coresNum = GetPlatform().CpuCores();
  TEST_GREATER(coresNum, 0, ());
  TEST_LESS_OR_EQUAL(coresNum, 128, ());
}

UNIT_TEST(VisualScale)
{
  double visualScale = GetPlatform().VisualScale();
  TEST_GREATER_OR_EQUAL(visualScale, 1.0, ());
}
