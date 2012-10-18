#include "platform.hpp"

#include "../base/logging.hpp"

#include <stdlib.h>
#include <unistd.h>


/// @return directory where binary resides, including slash at the end
static bool GetBinaryFolder(string & outPath)
{
  char path[4096] = {0};
  if (0 < ::readlink("/proc/self/exe", path, ARRAY_SIZE(path)))
  {
    outPath = path;
    outPath.erase(outPath.find_last_of('/') + 1);
    return true;
  }
  return false;
}

Platform::Platform()
{
  // init directories
  string path;
  CHECK(GetBinaryFolder(path), ("Can't retrieve path to executable"));

  // @TODO implement correct resources and writable directories for public releases
  m_resourcesDir = path + "../../data/";
  m_writableDir = m_resourcesDir;
  m_settingsDir = m_writableDir;
  char * tmpDir = ::getenv("TMPDIR");
  if (tmpDir)
    m_tmpDir = tmpDir;
  else
    m_tmpDir = "/tmp";
  m_tmpDir += '/';

  LOG(LDEBUG, ("Resources directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable directory:", m_writableDir));
  LOG(LDEBUG, ("Tmp directory:", m_tmpDir));
  LOG(LDEBUG, ("Settings directory:", m_settingsDir));
}

Platform::~Platform()
{
}

int Platform::CpuCores() const
{
  const long numCPU = sysconf(_SC_NPROCESSORS_ONLN);
  if (numCPU >= 1)
    return static_cast<int>(numCPU);
  return 1;
}

string Platform::UniqueClientId() const
{
  return "@TODO";
}

bool Platform::DoSupportRotation() const
{
  return false;
}

void Platform::RunOnGuiThread(TFunctor const & fn)
{
  /// @todo
  fn();
}

void Platform::RunAsync(TFunctor const & fn, Priority p)
{
  /// @todo
  fn();
}
