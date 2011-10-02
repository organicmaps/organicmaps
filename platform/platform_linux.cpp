#include "platform.hpp"

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define LOCALAPPDATA_DIR "MapsWithMe"

bool GetUserWritableDir(string & outDir)
{
  char * path = ::getenv("HOME");
  if (path)
  {
    outDir = path;
    outDir += "." LOCALAPPDATA_DIR "/";
    ::mkdir(outDir.c_str(), 0755);
    return true;
  }
  return false;
}

/// @return full path including binary itself
bool GetPathToBinary(string & outPath)
{
  char path[4096] = {0};
  if (0 < ::readlink("/proc/self/exe", path, ARRAY_SIZE(path)))
  {
    outPath = path;
    return true;
  }
  return false;
}

int Platform::CpuCores() const
{
  long numCPU = sysconf(_SC_NPROCESSORS_ONLN);
  if (numCPU >= 1)
    return static_cast<int>(numCPU);
  return 1;
}

string Platform::UniqueClientId() const
{
  return "@TODO";
}
