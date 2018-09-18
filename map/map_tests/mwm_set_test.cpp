#include "testing/testing.hpp"

#include "indexer/data_source.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/scope_guard.hpp"

#ifndef OMIM_OS_WINDOWS
#include <sys/stat.h>
#endif

using namespace base;
using namespace platform;

/*
 * This test is useless because of we don't build offsets index from now.
#ifndef OMIM_OS_WINDOWS
UNIT_TEST(MwmSet_FileSystemErrors)
{
  string const dir = GetPlatform().WritableDir();

  CountryFile file("minsk-pass");
  LocalCountryFile localFile(dir, file, 0);
  TEST(CountryIndexes::DeleteFromDisk(localFile), ());

  // Maximum level to check exception handling logic.
  LogLevel oldLevel = g_LogAbortLevel;
  g_LogAbortLevel = LCRITICAL;

  // Remove writable permission.
  int const readOnlyMode = S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH;
  TEST_EQUAL(chmod(dir.c_str(), readOnlyMode), 0, ());

  auto restoreFn = [oldLevel, &dir, readOnlyMode] ()
  {
    g_LogAbortLevel = oldLevel;
    TEST_EQUAL(chmod(dir.c_str(), readOnlyMode | S_IWUSR), 0, ());
  };
  SCOPE_GUARD(restoreGuard, restoreFn);

  DataSource dataSource;
  auto p = dataSource.RegisterMap(localFile);
  TEST_EQUAL(p.second, DataSource::RegResult::Success, ());

  // Registering should pass ok.
  TEST(dataSource.GetMwmIdByCountryFile(file) != DataSource::MwmId(), ());

  // Getting handle causes feature offsets index building which should fail
  // because of write permissions.
  TEST(!dataSource.GetMwmHandleById(p.first).IsAlive(), ());

  // Map is automatically deregistered after the fail.
  vector<shared_ptr<MwmInfo>> infos;
  dataSource.GetMwmsInfo(infos);
  TEST(infos.empty(), ());
}
#endif
*/
