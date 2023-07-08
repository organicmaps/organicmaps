#import "AppInfo.h"
#import "MWMCommon.h"

#include "platform/platform_ios.h"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include <sys/utsname.h>

#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>

@interface AppInfo ()

@property(nonatomic) NSString * bundleVersion;
@property(nonatomic) NSString * buildNumber;
@property(nonatomic) NSString * deviceModel;

@end

@implementation AppInfo

+ (instancetype)sharedInfo
{
  static AppInfo * appInfo;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    appInfo = [[self alloc] init];
  });
  return appInfo;
}

- (instancetype)init
{
  self = [super init];
  return self;
}

#pragma mark - Properties

- (NSString *)bundleVersion
{
  if (!_bundleVersion)
    _bundleVersion = NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"];
  return _bundleVersion;
}

- (NSString *)buildNumber
{
  if (!_buildNumber)
    _buildNumber = NSBundle.mainBundle.infoDictionary[@"CFBundleVersion"];
  return _buildNumber;
}

- (NSString *)languageId
{
  return NSLocale.preferredLanguages.firstObject;
}

- (NSString *)twoLetterLanguageId
{
  auto languageId = self.languageId;
  return languageId ? @(languages::Normalize(languageId.UTF8String).c_str()) : @"en";
}

- (NSString *)deviceModel
{
  if (!_deviceModel)
    _deviceModel = @(GetPlatform().DeviceModel().c_str());
  return _deviceModel;
}

- (MWMOpenGLDriver)openGLDriver
{
  utsname systemInfo;
  uname(&systemInfo);
  NSString * machine = @(systemInfo.machine);
  if (platform::kDeviceModelsBeforeMetalDriver[machine] != nil)
    return MWMOpenGLDriverRegular;
  if (platform::kDeviceModelsWithiOS10MetalDriver[machine] != nil)
  {
    if (isIOSVersionLessThan(10))
      return MWMOpenGLDriverRegular;
    else if (isIOSVersionLessThanString(@"10.3"))
      return MWMOpenGLDriverMetalPre103;
  }
  return MWMOpenGLDriverMetal;
}

- (BOOL)canMakeCalls
{
  if (UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPhone)
    return NO;
  NSURL * telURL = [NSURL URLWithString:@"tel://"];
  if (![UIApplication.sharedApplication canOpenURL:telURL])
    return NO;
  NSDictionary<NSString *,CTCarrier *> * dict = [[CTTelephonyNetworkInfo alloc] init].serviceSubscriberCellularProviders;
  for (id key in dict)
  {
    NSString * networkCode = [dict objectForKey:key].mobileNetworkCode;
    if (networkCode != nil && networkCode.length > 0 && ![networkCode isEqualToString:@"65535"])
      return YES;
  }
  return NO;
}

@end
