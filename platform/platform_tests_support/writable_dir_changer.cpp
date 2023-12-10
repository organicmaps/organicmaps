#include "testing/testing.hpp"

#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"


WritableDirChanger::WritableDirChanger(std::string const & testDir, bool m_settingsDir /* = false */)
  : m_writableDirBeforeTest(GetPlatform().WritableDir())
  , m_testDirFullPath(m_writableDirBeforeTest + testDir)
  , m_settingsDir(m_settingsDir)
{
  Platform & platform = GetPlatform();
  platform.RmDirRecursively(m_testDirFullPath);
  TEST(!platform.IsFileExistsByFullPath(m_testDirFullPath), ());
  TEST_EQUAL(Platform::ERR_OK, platform.MkDir(m_testDirFullPath), ());
  platform.SetWritableDirForTests(m_testDirFullPath);
  if (m_settingsDir)
  {
    platform.SetSettingsDir(m_testDirFullPath);
    /// @todo Make a copy of settings.ini file here?
  }
  settings::Clear();
}

WritableDirChanger::~WritableDirChanger()
{
  settings::Clear();
  Platform & platform = GetPlatform();
  std::string const writableDirForTest = platform.WritableDir();
  platform.SetWritableDirForTests(m_writableDirBeforeTest);
  if (m_settingsDir)
    platform.SetSettingsDir(m_writableDirBeforeTest);
  platform.RmDirRecursively(writableDirForTest);
}
