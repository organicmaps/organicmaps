#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSProcessInfo.h>
#import <UIKit/UIDevice.h>
#import <UIKit/UIScreen.h>
#import <UIKit/UIScreenMode.h>

#include "IPhonePlatform.hpp"

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

IPhonePlatform::IPhonePlatform()
{
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

  NSBundle * bundle = [NSBundle mainBundle];
  NSString * path = [bundle resourcePath];
  m_resourcesDir = [path UTF8String];
  m_resourcesDir += '/';

  NSArray * dirPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString * docsDir = [dirPaths objectAtIndex:0];
  m_writableDir = [docsDir UTF8String];
  m_writableDir += '/';

  /// Hardcoding screen resolution depending on the device we are running.
 	m_visualScale = 1.0;
  m_skinName = "basic.skn";

  /// Calculating resolution
  UIDevice * device = [UIDevice currentDevice];

  NSRange range = [device.name rangeOfString:@"iPad"];
  if (range.location != NSNotFound)
  {
    m_deviceID = "iPad";
    m_visualScale = 1.3;
	}
	else
  {
    range = [device.name rangeOfString:@"iPod"];
    float ver = [device.systemVersion floatValue];
    if (ver >= 3.1999)
    {
      m_deviceID = "iPod";
      UIScreen * mainScr = [UIScreen mainScreen];
      if (mainScr.currentMode.size.width == 640)
      {
        m_deviceID = "iPod";
        m_visualScale = 2.0;
        m_skinName = "basic_highres.skn";
      }
    }
  }

  m_scaleEtalonSize = (256 * 1.5) * m_visualScale;

  NSLog(@"Device Name : %@, SystemName : %@, SystemVersion : %@", device.name, device.systemName, device.systemVersion);

  [pool release];
}

IPhonePlatform::~IPhonePlatform()
{
}

void IPhonePlatform::GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles) const
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
        //    !(fileStatus.st_mode & S_IFDIR))
        //{
          outFiles.push_back(fname);
        //}
      }
    }
  } while (entry != NULL);

  closedir(dir);
}

bool IPhonePlatform::RenameFileX(string const & original, string const & newName) const
{
  return rename(original.c_str(), newName.c_str());
}

int IPhonePlatform::CpuCores() const
{
	NSInteger numCPU = [[NSProcessInfo processInfo] activeProcessorCount];
  if (numCPU >= 1)
  	return numCPU;
	return 1;
}

string IPhonePlatform::SkinName() const
{
	return m_skinName;
}

double IPhonePlatform::VisualScale() const
{
  return m_visualScale;
}

int IPhonePlatform::ScaleEtalonSize() const
{
	return m_scaleEtalonSize;
}

string IPhonePlatform::DeviceID() const
{
	return m_deviceID;
}

Platform & GetPlatform()
{
  static IPhonePlatform platform;
  return platform;
}
