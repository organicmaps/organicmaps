#include "../../testing/testing.hpp"

#include "../platform.hpp"
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

namespace
{
  template <int N>
  void TestForFiles(string const & dir, char const * (&arr)[N])
  {
    ASSERT( !dir.empty(), () );

    for (int i = 0; i < N; ++i)
    {
      FILE * f = fopen((dir + arr[i]).c_str(), "r");
      TEST_NOT_EQUAL(f, NULL, (arr[i], " is not present in ResourcesDir()"));
      if (f)
        fclose(f);
    }
  }
}

UNIT_TEST(WorkingDir)
{
  char const * arr[] = { "minsk-pass.dat", "minsk-pass.dat.idx"};
  TestForFiles(GetPlatform().WorkingDir(), arr);
}

UNIT_TEST(ResourcesDir)
{
  char const * arr[] = { "drawing_rules.bin", "symbols.png", "basic.skn", "classificator.txt" };
  TestForFiles(GetPlatform().ResourcesDir(), arr);
}

UNIT_TEST(GetFilesInDir)
{
  Platform & pl = GetPlatform();
  Platform::FilesList files;
  TEST_GREATER(pl.GetFilesInDir(pl.WorkingDir(), "*.dat", files), 0, ("/data/ folder should contain some *.dat files"));
}

UNIT_TEST(GetFileSize)
{
  Platform & pl = GetPlatform();
  uint64_t size = 0;
  pl.GetFileSize((pl.WorkingDir() + "minsk-pass.dat").c_str(), size);
  TEST_GREATER(size, 0, ("File /data/minsk-pass.dat should exist for test"));
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
