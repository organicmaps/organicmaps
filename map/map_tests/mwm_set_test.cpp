#include "testing/testing.hpp"

#include "indexer/index.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#ifndef OMIM_OS_WINDOWS
#include <sys/stat.h>
#endif


using namespace platform;
using namespace my;

#ifndef OMIM_OS_WINDOWS
UNIT_TEST(MwmSet_FileSystemErrors)
{
  string const dir = GetPlatform().WritableDir();

  CountryFile file("minsk-pass");
  LocalCountryFile localFile(dir, file, 0);
  TEST(CountryIndexes::DeleteFromDisk(localFile), ());

  LogLevel oldLevel = g_LogAbortLevel;
  g_LogAbortLevel = LCRITICAL;

  // Remove writable permission.
  int const readOnlyMode = S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH;
  TEST_EQUAL(chmod(dir.c_str(), readOnlyMode), 0, ());

  Index index;
  auto p = index.RegisterMap(localFile);
  TEST_EQUAL(p.second, Index::RegResult::Success, ());

  TEST(index.GetMwmIdByCountryFile(file) != Index::MwmId(), ());

  TEST(!index.GetMwmHandleById(p.first).IsAlive(), ());

  vector<shared_ptr<MwmInfo>> infos;
  index.GetMwmsInfo(infos);
  TEST(infos.empty(), ());

  // Restore writable permission.
  TEST_EQUAL(chmod(dir.c_str(), readOnlyMode | S_IWUSR), 0, ());

  g_LogAbortLevel = oldLevel;
}
#endif
