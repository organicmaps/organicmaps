#include "platform.hpp"

#include "../coding/file_writer.hpp"

#include "../std/windows.hpp"

#include <shlobj.h>
#include <sys/stat.h>

static bool GetUserWritableDir(string & outDir)
{
  char pathBuf[MAX_PATH] = {0};
  if (SUCCEEDED(::SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, pathBuf)))
  {
    outDir = pathBuf;
    ::CreateDirectoryA(outDir.c_str(), NULL);
    outDir += "\\MapsWithMe\\";
    ::CreateDirectoryA(outDir.c_str(), NULL);
    return true;
  }
  return false;
}

/// @return Full path to the executable file
static bool GetPathToBinary(string & outPath)
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

Platform::Platform()
{
  string path;
  CHECK(GetPathToBinary(path), ("Can't get path to binary"));

  // resources path:
  // 1. try to use data folder in the same path as executable
  // 2. if not found, try to use ..\..\..\data (for development only)
  path.erase(path.find_last_of('\\'));
  if (IsFileExists(path + "\\data\\"))
    m_resourcesDir = path + "\\data\\";
  else
  {
#ifndef OMIM_PRODUCTION
    path.erase(path.find_last_of('\\'));
    path.erase(path.find_last_of('\\'));
    if (IsFileExists(path + "\\data\\"))
      m_resourcesDir = path + "\\data\\";
#else
    CHECK(false, ("Can't find resources directory"));
#endif
  }

  // writable path:
  // 1. the same as resources if we have write access to this folder
  // 2. otherwise, use system-specific folder
  try
  {
    FileWriter tmpfile(m_resourcesDir + "mapswithmetmptestfile");
    tmpfile.Write("Hi from Alex!", 13);
    FileWriter::DeleteFileX(m_resourcesDir + "mapswithmetmptestfile");
    m_writableDir = m_resourcesDir;
  }
  catch (FileWriter::RootException const &)
  {
    CHECK(GetUserWritableDir(m_writableDir), ("Can't get writable directory"));
  }

  LOG(LDEBUG, ("Resources Directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable Directory:", m_writableDir));
}

Platform::~Platform()
{
}

bool Platform::IsFileExists(string const & file) const
{
  struct _stat s;
  return _stat(file.c_str(), &s) == 0;
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
