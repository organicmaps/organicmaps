#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "std/target_os.hpp"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include <IOKit/IOKitLib.h>
#include <Foundation/NSBundle.h>
#include <Foundation/NSPathUtilities.h>
#include <Foundation/NSAutoreleasePool.h>

#include <sys/stat.h>
#include <sys/sysctl.h>

#include <dispatch/dispatch.h>

#import <SystemConfiguration/SystemConfiguration.h>
#import <netinet/in.h>

Platform::Platform()
{
  // get resources directory path
  string const resourcesPath = [[[NSBundle mainBundle] resourcePath] UTF8String];
  string const bundlePath = [[[NSBundle mainBundle] bundlePath] UTF8String];
  if (resourcesPath == bundlePath)
  {
    // we're the console app, probably unit test, and path is our directory
    m_resourcesDir = bundlePath + "/../../data/";
    if (!IsFileExistsByFullPath(m_resourcesDir))
    {
      // Check development environment without symlink but with git repo
      string const repoPath = bundlePath + "/../../../omim/data/";
      if (IsFileExistsByFullPath(repoPath))
        m_resourcesDir = repoPath;
      else
        m_resourcesDir = "./data/";
    }
    m_writableDir = m_resourcesDir;
  }
  else
  {
    m_resourcesDir = resourcesPath + "/";

    // get writable path
    // developers can have symlink to data folder
    char const * dataPath = "../../../../../data/";
    if (IsFileExistsByFullPath(m_resourcesDir + dataPath))
      m_writableDir = m_resourcesDir + dataPath;
    else
    {
      // Check development environment without symlink but with git repo
      dataPath = "../../../../../../omim/data/";
      if (IsFileExistsByFullPath(m_resourcesDir + dataPath))
        m_writableDir = m_resourcesDir + dataPath;
    }

    if (m_writableDir.empty())
    {
      NSArray * dirPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
      NSString * supportDir = [dirPaths objectAtIndex:0];
      m_writableDir = [supportDir UTF8String];
      m_writableDir += "/MapsWithMe/";
      ::mkdir(m_writableDir.c_str(), 0755);
    }
  }

  m_settingsDir = m_writableDir;

  NSString * tempDir = NSTemporaryDirectory();
  if (tempDir == nil)
      tempDir = @"/tmp";
  m_tmpDir = [tempDir UTF8String];
  m_tmpDir += '/';

  LOG(LDEBUG, ("Resources Directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable Directory:", m_writableDir));
  LOG(LDEBUG, ("Tmp Directory:", m_tmpDir));
  LOG(LDEBUG, ("Settings Directory:", m_settingsDir));
}

string Platform::UniqueClientId() const { return [Alohalytics installationId].UTF8String; }

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
    case EPriorityDefault: priority = DISPATCH_QUEUE_PRIORITY_DEFAULT; break;
    case EPriorityHigh: priority = DISPATCH_QUEUE_PRIORITY_HIGH; break;
    case EPriorityLow: priority = DISPATCH_QUEUE_PRIORITY_LOW; break;
    // It seems like this option is not supported in Snow Leopard.
    //case EPriorityBackground: priority = DISPATCH_QUEUE_PRIORITY_BACKGROUND; break;
    default: priority = INT16_MIN;
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
  return EConnectionType::CONNECTION_WIFI;
}
