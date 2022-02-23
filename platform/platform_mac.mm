#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "std/target_os.hpp"

#include <memory>
#include <string>
#include <utility>

#include <Foundation/NSAutoreleasePool.h>
#include <Foundation/NSBundle.h>
#include <Foundation/NSFileManager.h>
#include <Foundation/NSPathUtilities.h>
#include <IOKit/IOKitLib.h>

#include <sys/stat.h>
#include <sys/sysctl.h>

#include <dispatch/dispatch.h>

#import <SystemConfiguration/SystemConfiguration.h>
#import <netinet/in.h>

namespace
{

template <class FnT> void ForEachPossibleDir(std::string const & basePath, FnT && fn)
{
  std::string const dirs[] =
  {
    // Check symlink in the current folder (preferred environment setup).
    basePath + "/data",
    // Check if we are in the 'build' folder inside repo.
    basePath + "/../data",
    // Check if we are near with default repo's name.
    basePath + "/../organicmaps/data",
  };

  for (auto const & dir : dirs)
  {
    if (Platform::IsFileExistsByFullPath(dir))
    {
      fn(dir);
      return;
    }
  }
}

} // namespace

Platform::Platform()
{
  // get resources directory path
  std::string const resourcesPath = NSBundle.mainBundle.resourcePath.UTF8String;
  std::string const bundlePath = NSBundle.mainBundle.bundlePath.UTF8String;

  char const * envResourcesDir = ::getenv("MWM_RESOURCES_DIR");
  char const * envWritableDir = ::getenv("MWM_WRITABLE_DIR");

  if (envResourcesDir && envWritableDir)
  {
    m_resourcesDir = envResourcesDir;
    m_writableDir = envWritableDir;
  }
  else if (resourcesPath == bundlePath)
  {
    // We're the console app (tests or tools), and resource path is a running directory.
    ForEachPossibleDir(resourcesPath, [this](std::string const & dir)
    {
      m_resourcesDir = m_writableDir = dir;
    });
  }
  else
  {
    // We are the bundled executable with its own resources.
    m_resourcesDir = resourcesPath;
    ForEachPossibleDir(resourcesPath + "/../../..", [this](std::string const & dir)
    {
      m_writableDir = dir;
    });

    if (m_writableDir.empty())
    {
      NSArray * dirPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
      NSString * supportDir = [dirPaths objectAtIndex:0];
      m_writableDir = supportDir.UTF8String;
#ifdef BUILD_DESIGNER
      m_writableDir += "/OMapsData.Designer";
#else // BUILD_DESIGNER
      m_writableDir += "/OMapsData";
#endif // BUILD_DESIGNER
      ::mkdir(m_writableDir.c_str(), 0755);
    }
  }

  ValidateWritableAndResourceDirs();

  m_settingsDir = m_writableDir;

  NSString * tempDir = NSTemporaryDirectory();
  if (tempDir == nil)
    tempDir = @"/tmp";
  m_tmpDir = tempDir.UTF8String;
  m_tmpDir += '/';

  m_guiThread = std::make_unique<platform::GuiThread>();

  LOG(LDEBUG, ("Resources Directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable Directory:", m_writableDir));
  LOG(LDEBUG, ("Tmp Directory:", m_tmpDir));
  LOG(LDEBUG, ("Settings Directory:", m_settingsDir));
}

std::string Platform::DeviceName() const
{
  return OMIM_OS_NAME;
}

std::string Platform::DeviceModel() const
{
  return {};
}

Platform::EConnectionType Platform::ConnectionStatus()
{
  struct sockaddr_in zero;
  bzero(&zero, sizeof(zero));
  zero.sin_len = sizeof(zero);
  zero.sin_family = AF_INET;
  SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, reinterpret_cast<const struct sockaddr*>(&zero));
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

// static
Platform::ChargingStatus Platform::GetChargingStatus()
{
  return Platform::ChargingStatus::Plugged;
}

uint8_t Platform::GetBatteryLevel()
{
  // This value is always 100 for desktop.
  return 100;
}

void Platform::GetSystemFontNames(FilesList & res) const
{
}
