#include "testing/testing.hpp"

#include "write_dir_changer.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"

WritableDirChanger::WritableDirChanger(string const & testDir)
  : m_writableDirBeforeTest(GetPlatform().WritableDir())
  , m_testDirFullPath(m_writableDirBeforeTest + testDir)
{
  Platform & platform = GetPlatform();
  platform.RmDirRecursively(m_testDirFullPath);
  TEST(!platform.IsFileExistsByFullPath(m_testDirFullPath), ());
  TEST_EQUAL(Platform::ERR_OK, platform.MkDir(m_testDirFullPath), ());
  platform.SetWritableDirForTests(m_testDirFullPath);
  Settings::Clear();
}

WritableDirChanger::~WritableDirChanger()
{
  Settings::Clear();
  Platform & platform = GetPlatform();
  string const writableDirForTest = platform.WritableDir();
  platform.SetWritableDirForTests(m_writableDirBeforeTest);
  platform.RmDirRecursively(writableDirForTest);
}
