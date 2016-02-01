#include "platform/constants.hpp"
#include "platform/platform.hpp"
#include "platform/platform_unix_impl.hpp"
#include "platform/settings.hpp"

#include "coding/file_reader.hpp"

#include <ifaddrs.h>

#include <mach/mach.h>

#include <net/if_dl.h>
#include <net/if.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(IFT_ETHER)
  #define IFT_ETHER 0x6 /* Ethernet CSMACD */
#endif

#import "../iphone/Maps/Classes/Common.h"

#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSProcessInfo.h>

#import <UIKit/UIDevice.h>
#import <UIKit/UIScreen.h>
#import <UIKit/UIScreenMode.h>

#import <SystemConfiguration/SystemConfiguration.h>
#import <netinet/in.h>

namespace
{
void migrateFromSharedGroupIfRequired()
{
  NSUserDefaults * settings =
      [[NSUserDefaults alloc] initWithSuiteName:kApplicationGroupIdentifier()];
  if (![settings boolForKey:kHaveAppleWatch])
    return;

  NSFileManager * fileManager = [NSFileManager defaultManager];
  NSURL * privateURL =
      [fileManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
  NSURL * sharedURL =
      [fileManager containerURLForSecurityApplicationGroupIdentifier:kApplicationGroupIdentifier()];

  NSDirectoryEnumerator * dirEnum =
      [fileManager enumeratorAtURL:sharedURL
          includingPropertiesForKeys:nil
                             options:NSDirectoryEnumerationSkipsSubdirectoryDescendants |
                                     NSDirectoryEnumerationSkipsPackageDescendants |
                                     NSDirectoryEnumerationSkipsHiddenFiles
                        errorHandler:nil];

  NSURL * sourceURL = nil;
  while ((sourceURL = [dirEnum nextObject]))
  {
    NSString * fileName = [sourceURL lastPathComponent];
    NSURL * destinationURL = [privateURL URLByAppendingPathComponent:fileName];
    NSError * error = nil;
    [fileManager moveItemAtURL:sourceURL toURL:destinationURL error:&error];
    if (error && [error.domain isEqualToString:NSCocoaErrorDomain] &&
        error.code == NSFileWriteFileExistsError)
    {
      [fileManager removeItemAtURL:destinationURL error:nil];
      [fileManager moveItemAtURL:sourceURL toURL:destinationURL error:nil];
    }
  }

  [settings setBool:NO forKey:kHaveAppleWatch];
  [settings synchronize];
}
}  // namespace

Platform::Platform()
{
  migrateFromSharedGroupIfRequired();
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

  m_isTablet = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);

  NSBundle * bundle = [NSBundle mainBundle];
  NSString * path = [bundle resourcePath];
  m_resourcesDir = [path UTF8String];
  m_resourcesDir += "/";

  NSArray * dirPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString * docsDir = [dirPaths firstObject];
  m_writableDir = [docsDir UTF8String];
  m_writableDir += "/";
  m_settingsDir = m_writableDir;

  NSString * tmpDir = NSTemporaryDirectory();
  if (tmpDir)
    m_tmpDir = [tmpDir UTF8String];
  else
  {
    m_tmpDir = [NSHomeDirectory() UTF8String];
    m_tmpDir += "/tmp/";
  }

  UIDevice * device = [UIDevice currentDevice];
  NSLog(@"Device: %@, SystemName: %@, SystemVersion: %@", device.model, device.systemName, device.systemVersion);

  [pool release];
}

Platform::EError Platform::MkDir(string const & dirName) const
{
  if (::mkdir(dirName.c_str(), 0755))
    return ErrnoToError();
  return Platform::ERR_OK;
}

void Platform::GetFilesByRegExp(string const & directory, string const & regexp, FilesList & res)
{
  pl::EnumerateFilesByRegExp(directory, regexp, res);
}

bool Platform::GetFileSizeByName(string const & fileName, uint64_t & size) const
{
  try
  {
    return GetFileSizeByFullPath(ReadPathForFile(fileName), size);
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

string Platform::GetMemoryInfo() const
{
  struct task_basic_info info;
  mach_msg_type_number_t size = sizeof(info);
  kern_return_t const kerr = task_info(mach_task_self(),
                                 TASK_BASIC_INFO,
                                 (task_info_t)&info,
                                 &size);
  stringstream ss;
  if (kerr == KERN_SUCCESS)
  {
    ss << "Memory info: Resident_size = " << info.resident_size / 1024
      << "KB; virtual_size = " << info.resident_size / 1024 << "KB; suspend_count = " << info.suspend_count
      << " policy = " << info.policy;
  }
  else
  {
    ss << "Error with task_info(): " << mach_error_string(kerr);
  }
  return ss.str();
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

Platform::EConnectionType Platform::ConnectionStatus()
{
  struct sockaddr_in zero;
  bzero(&zero, sizeof(zero));
  zero.sin_len = sizeof(zero);
  zero.sin_family = AF_INET;
  SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr*)&zero);
  if (!reachability)
    return EConnectionType::CONNECTION_NONE;
  SCNetworkReachabilityFlags flags;
  bool const gotFlags = SCNetworkReachabilityGetFlags(reachability, &flags);
  CFRelease(reachability);
  if (!gotFlags || ((flags & kSCNetworkReachabilityFlagsReachable) == 0))
    return EConnectionType::CONNECTION_NONE;
  SCNetworkReachabilityFlags userActionRequired = kSCNetworkReachabilityFlagsConnectionRequired | kSCNetworkReachabilityFlagsInterventionRequired;
  if ((flags & userActionRequired) == userActionRequired)
    return EConnectionType::CONNECTION_NONE;
  if ((flags & kSCNetworkReachabilityFlagsIsWWAN) == kSCNetworkReachabilityFlagsIsWWAN)
    return EConnectionType::CONNECTION_WWAN;
  else
    return EConnectionType::CONNECTION_WIFI;
}

void Platform::SetupMeasurementSystem() const
{
  Settings::Units u;
  if (Settings::Get("Units", u))
    return;
  BOOL const isMetric = [[[NSLocale autoupdatingCurrentLocale] objectForKey:NSLocaleUsesMetricSystem] boolValue];
  u = isMetric ? Settings::Metric : Settings::Foot;
  Settings::Set("Units", u);
}

////////////////////////////////////////////////////////////////////////
extern Platform & GetPlatform()
{
  static Platform platform;
  return platform;
}
