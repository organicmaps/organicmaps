#include "platform.hpp"

#include "../base/logging.hpp"

#include "../coding/sha2.hpp"
#include "../coding/base64.hpp"

#include "../std/target_os.hpp"

#include <IOKit/IOKitLib.h>
#include <Foundation/NSBundle.h>
#include <Foundation/NSPathUtilities.h>
#include <Foundation/NSAutoreleasePool.h>

#include <sys/stat.h>
#include <sys/sysctl.h>

#include <dispatch/dispatch.h>


Platform::Platform()
{
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

  // get resources directory path
  string const resourcesPath = [[[NSBundle mainBundle] resourcePath] UTF8String];
  string const bundlePath = [[[NSBundle mainBundle] bundlePath] UTF8String];
  if (resourcesPath == bundlePath)
  {
    // we're the console app, probably unit test, and path is our directory
    m_resourcesDir = bundlePath + "/../../data/";
    m_writableDir = m_resourcesDir;
  }
  else
  {
    m_resourcesDir = resourcesPath + "/";

    // get writable path
#ifndef OMIM_PRODUCTION
    // developers can have symlink to data folder
    char const * dataPath = "../../../../../data/";
    if (IsFileExistsByFullPath(m_resourcesDir + dataPath))
      m_writableDir = m_resourcesDir + dataPath;
#endif

    if (m_writableDir.empty())
    {
      NSArray * dirPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
      NSString * supportDir = [dirPaths objectAtIndex:0];
      m_writableDir = [supportDir UTF8String];
      m_writableDir += "/MapsWithMe/";
      ::mkdir(m_writableDir.c_str(), 0755);
    }
  }
  [pool release];

  LOG(LDEBUG, ("Resources Directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable Directory:", m_writableDir));
}

Platform::~Platform()
{
}

bool Platform::IsFileExistsByFullPath(string const & filePath)
{
  struct stat s;
  return stat(filePath.c_str(), &s) == 0;
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
  CFStringGetCString(uuidCf, buf, 512, kCFStringEncodingUTF8);
  CFRelease(uuidCf);

  // generate sha2 hash for UUID
  string const hash = sha2::digest256(buf, false);
  // xor it
  size_t const offset = hash.size() / 4;
  string xoredHash;
  for (size_t i = 0; i < offset; ++i)
    xoredHash.push_back(hash[i] ^ hash[i + offset] ^ hash[i + offset * 2] ^ hash[i + offset * 3]);
  // and use base64 encoding
  return base64::encode(xoredHash);
}

static void PerformImpl(void * obj)
{
  Platform::TFunctor * f = reinterpret_cast<Platform::TFunctor *>(obj);
  (*f)();
  delete f;
}

void Platform::RunOnGuiThread(TFunctor const & fn)
{
  dispatch_async_f(dispatch_get_main_queue(), new TFunctor(fn), &PerformImpl);
}

void Platform::RunAsync(TFunctor const & fn, Priority p)
{
  int priority = DISPATCH_QUEUE_PRIORITY_DEFAULT;
  switch (p)
  {
    // It seems like this option is not supported in Snow Leopard.
    //case EPriorityBackground: priority = DISPATCH_QUEUE_PRIORITY_BACKGROUND; break;
    case EPriorityDefault: priority = DISPATCH_QUEUE_PRIORITY_DEFAULT; break;
    case EPriorityHigh: priority = DISPATCH_QUEUE_PRIORITY_HIGH; break;
    case EPriorityLow: priority = DISPATCH_QUEUE_PRIORITY_LOW; break;
  }
  dispatch_async_f(dispatch_get_global_queue(priority, 0), new TFunctor(fn), &PerformImpl);
}
