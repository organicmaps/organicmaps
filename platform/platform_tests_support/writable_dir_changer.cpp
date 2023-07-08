#include "testing/testing.hpp"

#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/internal/file_data.hpp"

#include "base/file_name_utils.hpp"

WritableDirChanger::WritableDirChanger(std::string const & testDir, SettingsDirPolicy settingsDirPolicy)
  : m_writableDirBeforeTest(GetPlatform().WritableDir())
  , m_testDirFullPath(m_writableDirBeforeTest + testDir)
  , m_settingsDirPolicy(settingsDirPolicy)
{
  Platform & platform = GetPlatform();
  platform.RmDirRecursively(m_testDirFullPath);
  TEST(!platform.IsFileExistsByFullPath(m_testDirFullPath), ());
  TEST_EQUAL(Platform::ERR_OK, platform.MkDir(m_testDirFullPath), ());
  platform.SetWritableDirForTests(m_testDirFullPath);
  if (m_settingsDirPolicy == SettingsDirPolicy::UseWritableDir)
    platform.SetSettingsDir(m_testDirFullPath);
  settings::Clear();
}

WritableDirChanger::~WritableDirChanger()
{
  settings::Clear();
  Platform & platform = GetPlatform();
  std::string const writableDirForTest = platform.WritableDir();
  platform.SetWritableDirForTests(m_writableDirBeforeTest);
  if (m_settingsDirPolicy == SettingsDirPolicy::UseWritableDir)
    platform.SetSettingsDir(m_writableDirBeforeTest);
  platform.RmDirRecursively(writableDirForTest);
}
