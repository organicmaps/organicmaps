#pragma once

#include <string>

class WritableDirChanger
{
public:
  explicit WritableDirChanger(std::string const & testDir, bool setSettingsDir = false);
  ~WritableDirChanger();

private:
  std::string const m_writableDirBeforeTest;
  std::string const m_testDirFullPath;
  bool m_settingsDir;
};
