#include "platform.hpp"

#include "../std/windows.hpp"

#include <shlobj.h>

#define LOCALAPPDATA_DIR "MapsWithMe"

bool GetUserWritableDir(string & outDir)
{
  char pathBuf[MAX_PATH] = {0};
  if (SUCCEEDED(::SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, pathBuf)))
  {
    outDir = pathBuf;
    ::CreateDirectoryA(outDir.c_str(), NULL);
    outDir += "\\" LOCALAPPDATA_DIR "\\";
    ::CreateDirectoryA(outDir.c_str(), NULL);
    return true;
  }
  return false;
}

/// @return full path including binary itself
bool GetPathToBinary(string & outPath)
{
  // get path to executable
  char pathBuf[MAX_PATH] = {0};
  if (0 < ::GetModuleFileNameA(NULL, pathBuf, MAX_PATH))
  {
    outPath = pathBuf;
    return true;
  }
  return false;
}

int Platform::CpuCores() const
{
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  DWORD numCPU = sysinfo.dwNumberOfProcessors;
  if (numCPU >= 1)
    return static_cast<int>(numCPU);
  return 1;
}

string Platform::UniqueClientId() const
{
  return "@TODO";
}
