#include "std/string.hpp"

class WritableDirChanger
{
public:
  WritableDirChanger(string testDir);
  ~WritableDirChanger();

private:
  string const m_writableDirBeforeTest;
  string const m_testDirFullPath;
};
