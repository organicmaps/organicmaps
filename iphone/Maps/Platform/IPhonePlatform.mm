#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSProcessInfo.h>

#import <UIKit/UIDevice.h>
#import <UIKit/UIScreen.h>
#import <UIKit/UIScreenMode.h>

#include "IPhonePlatform.hpp"


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

bool IPhonePlatform::IsMultiSampled() const
{
  return false;
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
