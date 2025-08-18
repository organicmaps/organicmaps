#pragma once

#include <string>

class WritableDirChanger
{
public:
  enum class SettingsDirPolicy
  {
    UseDefault,
    UseWritableDir
  };

  WritableDirChanger(std::string const & testDir, SettingsDirPolicy settingsDirPolicy = SettingsDirPolicy::UseDefault);
  ~WritableDirChanger();

private:
  std::string const m_writableDirBeforeTest;
  std::string const m_testDirFullPath;
  SettingsDirPolicy m_settingsDirPolicy;
};
