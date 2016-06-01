#include "std/string.hpp"

class WritableDirChanger
{
public:
  enum class SettingsDirPolicy
  {
    UseDefault, UseWritableDir
  };

  WritableDirChanger(string const & testDir,
                     SettingsDirPolicy settingsDirPolicy = SettingsDirPolicy::UseDefault);
  ~WritableDirChanger();

private:
  string const m_writableDirBeforeTest;
  string const m_testDirFullPath;
  SettingsDirPolicy m_settingsDirPolicy;
};
