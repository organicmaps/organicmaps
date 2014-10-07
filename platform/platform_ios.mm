#include "platform.hpp"
#include "platform_unix_impl.hpp"
#include "constants.hpp"

#include "../coding/file_reader.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <net/if.h>

#if !defined(IFT_ETHER)
  #define IFT_ETHER 0x6 /* Ethernet CSMACD */
#endif

#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSProcessInfo.h>

#import <UIKit/UIDevice.h>
#import <UIKit/UIScreen.h>
#import <UIKit/UIScreenMode.h>


Platform::Platform()
{
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

  NSBundle * bundle = [NSBundle mainBundle];
  NSString * path = [bundle resourcePath];
  m_resourcesDir = [path UTF8String];
  m_resourcesDir += "/";

  NSArray * dirPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString * docsDir = [dirPaths objectAtIndex:0];
  m_writableDir = [docsDir UTF8String];
  m_writableDir += "/";
  m_settingsDir = m_writableDir;
  m_tmpDir = [NSHomeDirectory() UTF8String];
  m_tmpDir += "/tmp/";

  NSString * appID = [[bundle infoDictionary] objectForKey:@"CFBundleIdentifier"];

  // .travelguide corresponds to the Lite version without search
  m_flags[PRO_URL] = ([appID rangeOfString:@"com.mapswithme.travelguide"].location == NSNotFound);
  m_flags[HAS_BOOKMARKS] = m_flags[HAS_ROTATION] = m_flags[HAS_ROUTING] = m_flags[PRO_URL];

  UIDevice * device = [UIDevice currentDevice];
  NSLog(@"Device: %@, SystemName: %@, SystemVersion: %@", device.model, device.systemName, device.systemVersion);

  [pool release];
}

void Platform::GetFilesByRegExp(string const & directory, string const & regexp, FilesList & res)
{
  pl::EnumerateFilesByRegExp(directory, regexp, res);
}

bool Platform::GetFileSizeByName(string const & fileName, uint64_t & size) const
{
  try
  {
    return GetFileSizeByFullPath(ReadPathForFile(fileName, "wr"), size);
  }
  catch (RootException const &)
  {
    return false;
  }
}

ModelReader * Platform::GetReader(string const & file, string const & searchScope) const
{
  return new FileReader(ReadPathForFile(file, searchScope),
                        READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
}

int Platform::CpuCores() const
{
  NSInteger const numCPU = [[NSProcessInfo processInfo] activeProcessorCount];
  if (numCPU >= 1)
    return numCPU;
  return 1;
}

int Platform::VideoMemoryLimit() const
{
  return 8 * 1024 * 1024;
}

int Platform::PreCachingDepth() const
{
  return 2;
}

static string GetDeviceUid()
{
  NSString * uid = @"";
  UIDevice * device = [UIDevice currentDevice];
  if (device.systemVersion.floatValue >= 6.0 && device.identifierForVendor)
    uid = [device.identifierForVendor UUIDString];
  return [uid UTF8String];
}

static string GetMacAddress()
{
  string result;
  // get wifi mac addr
  ifaddrs * addresses = NULL;
  if (getifaddrs(&addresses) == 0 && addresses != NULL)
  {
    ifaddrs * currentAddr = addresses;
    do
    {
      if (currentAddr->ifa_addr->sa_family == AF_LINK
          && ((const struct sockaddr_dl *) currentAddr->ifa_addr)->sdl_type == IFT_ETHER)
      {
        const struct sockaddr_dl * dlAddr = (const struct sockaddr_dl *) currentAddr->ifa_addr;
        const char * base = &dlAddr->sdl_data[dlAddr->sdl_nlen];
        result.assign(base, dlAddr->sdl_alen);
        break;
      }
      currentAddr = currentAddr->ifa_next;
    }
    while (currentAddr->ifa_next);
    freeifaddrs(addresses);
  }
  return result;
}

string Platform::UniqueClientId() const
{
  return HashUniqueID(GetMacAddress() + GetDeviceUid());
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
    case EPriorityBackground: priority = DISPATCH_QUEUE_PRIORITY_BACKGROUND; break;
    case EPriorityDefault: priority = DISPATCH_QUEUE_PRIORITY_DEFAULT; break;
    case EPriorityHigh: priority = DISPATCH_QUEUE_PRIORITY_HIGH; break;
    case EPriorityLow: priority = DISPATCH_QUEUE_PRIORITY_LOW; break;
  }
  dispatch_async_f(dispatch_get_global_queue(priority, 0), new TFunctor(fn), &PerformImpl);
}

////////////////////////////////////////////////////////////////////////
extern Platform & GetPlatform()
{
  static Platform platform;
  return platform;
}
