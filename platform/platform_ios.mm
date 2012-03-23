#include "platform.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/base64.hpp"
#include "../coding/sha2.hpp"

#include <dirent.h>
#include <sys/stat.h>
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

class Platform::PlatformImpl
{
public:
  double m_visualScale;
  int m_scaleEtalonSize;
  string m_skinName;
  string m_deviceName;
  int m_videoMemoryLimit;
};

Platform::Platform()
{
  m_impl = new PlatformImpl;

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

  m_impl->m_visualScale = [[UIScreen mainScreen] scale];
  if (m_impl->m_visualScale > 1.0)
    m_impl->m_skinName = "basic_xhdpi.skn";
  else
    m_impl->m_skinName = "basic_mdpi.skn";

  m_impl->m_videoMemoryLimit = 8 * 1024 * 1024;

  UIDevice * device = [UIDevice currentDevice];
  NSRange range = [device.model rangeOfString:@"iPad"];
  if (range.location != NSNotFound)
    m_impl->m_deviceName = "iPad";
  else
  {
    range = [device.model rangeOfString:@"iPod"];
    if (range.location != NSNotFound)
      m_impl->m_deviceName = "iPod";
    else
      m_impl->m_deviceName = "iPhone";
  }

  m_impl->m_scaleEtalonSize = 256 * 1.5 * m_impl->m_visualScale;

  NSLog(@"Device: %@, SystemName: %@, SystemVersion: %@", device.model, device.systemName, device.systemVersion);

  [pool release];
}

Platform::~Platform()
{
  delete m_impl;
}

bool Platform::IsFileExistsByFullPath(string const & filePath)
{
  struct stat s;
  return stat(filePath.c_str(), &s) == 0;
}

void Platform::GetFilesInDir(string const & directory, string const & mask, FilesList & res)
{
  DIR * dir;
  struct dirent * entry;

  if ((dir = opendir(directory.c_str())) == NULL)
    return;

  // TODO: take wildcards into account...
  string mask_fixed = mask;
  if (mask_fixed.size() && mask_fixed[0] == '*')
    mask_fixed.erase(0, 1);

  do
  {
    if ((entry = readdir(dir)) != NULL)
    {
      string fname(entry->d_name);
      size_t index = fname.rfind(mask_fixed);
      if (index != string::npos && index == fname.size() - mask_fixed.size())
      {
        // TODO: By some strange reason under simulator stat returns -1,
        // may be because of symbolic links?..
        //struct stat fileStatus;
        //if (stat(string(directory + fname).c_str(), &fileStatus) == 0 &&
        //    (fileStatus.st_mode & S_IFDIR) == 0)
        //{
          res.push_back(fname);
        //}
      }
    }
  } while (entry != NULL);

  closedir(dir);
}

bool Platform::GetFileSizeByFullPath(string const & filePath, uint64_t & size)
{
  struct stat s;
  if (stat(filePath.c_str(), &s) == 0)
  {
    size = s.st_size;
    return true;
  }
  return false;
}

bool Platform::GetFileSizeByName(string const & fileName, uint64_t & size) const
{
  try
  {
    return GetFileSizeByFullPath(ReadPathForFile(fileName), size);
  }
  catch (std::exception const &)
  {
    return false;
  }
}

void Platform::GetFontNames(FilesList & res) const
{
  GetFilesInDir(ResourcesDir(), "*.ttf", res);
  sort(res.begin(), res.end());
}

ModelReader * Platform::GetReader(string const & file) const
{
  return new FileReader(ReadPathForFile(file), 10, 12);
}

int Platform::CpuCores() const
{
  NSInteger const numCPU = [[NSProcessInfo processInfo] activeProcessorCount];
  if (numCPU >= 1)
    return numCPU;
  return 1;
}

string Platform::SkinName() const
{
  return m_impl->m_skinName;
}

double Platform::VisualScale() const
{
  return m_impl->m_visualScale;
}

int Platform::ScaleEtalonSize() const
{
  return m_impl->m_scaleEtalonSize;
}

int Platform::VideoMemoryLimit() const
{
  return m_impl->m_videoMemoryLimit;
}

int Platform::PreCachingDepth() const
{
  return 2;
}

int Platform::TileSize() const
{
  return 512;
}

string Platform::DeviceName() const
{
  return m_impl->m_deviceName;
}

static string GetDeviceUid()
{
  NSString * uid = [[UIDevice currentDevice] uniqueIdentifier];
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

static string GetUniqueHashedId()
{
  // generate sha2 hash for mac address
  string const hash = sha2::digest256(GetMacAddress() + GetDeviceUid(), false);
  // xor it
  size_t const offset = hash.size() / 4;
  string xoredHash;
  for (size_t i = 0; i < offset; ++i)
    xoredHash.push_back(hash[i] ^ hash[i + offset] ^ hash[i + offset * 2] ^ hash[i + offset * 3]);
  // and use base64 encoding
  return base64::encode(xoredHash);
}

string Platform::UniqueClientId() const
{
  return GetUniqueHashedId();
}

bool Platform::IsFeatureSupported(string const & feature) const
{
  if (feature == "search")
  {
    NSString * appID = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleIdentifier"];
    // .travelguide corresponds to the Lite version without search
    if ([appID rangeOfString:@"com.mapswithme.travelguide"].location != NSNotFound)
      return false;
    return true;
  }
  return false;
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
extern "C" Platform & GetPlatform()
{
  static Platform platform;
  return platform;
}
