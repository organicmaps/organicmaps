#include "platform.hpp"

#include "../std/target_os.hpp"

#include <glob.h>
#include <NSSystemDirectories.h>
#include <mach-o/dyld.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <IOKit/IOKitLib.h>

#define LOCALAPPDATA_DIR "MapsWithMe"

static string ExpandTildePath(char const * path)
{
  glob_t globbuf;
  string result = path;

  if (::glob(path, GLOB_TILDE, NULL, &globbuf) == 0) //success
  {
    if (globbuf.gl_pathc > 0)
      result = globbuf.gl_pathv[0];

    globfree(&globbuf);
  }
  return result;
}

bool GetUserWritableDir(string & outDir)
{
  char pathBuf[PATH_MAX];
  NSSearchPathEnumerationState state = ::NSStartSearchPathEnumeration(NSApplicationSupportDirectory, NSUserDomainMask);
  while ((state = NSGetNextSearchPathEnumeration(state, pathBuf)))
  {
    outDir = ExpandTildePath(pathBuf);
    ::mkdir(outDir.c_str(), 0755);
    outDir += "/" LOCALAPPDATA_DIR "/";
    ::mkdir(outDir.c_str(), 0755);
    return true;
  }
  return false;
}

/// @return full path including binary itself
bool GetPathToBinary(string & outPath)
{
  char path[MAXPATHLEN] = {0};
  uint32_t pathSize = ARRAY_SIZE(path);
  if (0 == ::_NSGetExecutablePath(path, &pathSize))
  {
    char fixedPath[MAXPATHLEN] = {0};
    if (::realpath(path, fixedPath))
    {
      outPath = fixedPath;
      return true;
    }
  }
  return false;
}

int Platform::CpuCores() const
{
  int mib[2], numCPU = 0;
  size_t len = sizeof(numCPU);
  mib[0] = CTL_HW;
  mib[1] = HW_AVAILCPU;
  sysctl(mib, 2, &numCPU, &len, NULL, 0);
  if (numCPU >= 1)
    return numCPU;
  // second try
  mib[1] = HW_NCPU;
  len = sizeof(numCPU);
  sysctl(mib, 2, &numCPU, &len, NULL, 0);
  if (numCPU >= 1)
    return numCPU;
  return 1;
}

string Platform::UniqueClientId() const
{
  io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
  CFStringRef uuidCf = (CFStringRef) IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
  IOObjectRelease(ioRegistryRoot);
  char buf[513];
  CFStringGetCString(uuidCf, buf, 512, kCFStringEncodingMacRoman);
  CFRelease(uuidCf);
  return buf;
}
