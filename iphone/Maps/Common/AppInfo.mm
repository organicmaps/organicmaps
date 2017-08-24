#import "AppInfo.h"
#import <AdSupport/ASIdentifierManager.h>
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <sys/utsname.h>
#import "MWMCommon.h"

#include "platform/settings.hpp"

extern string const kCountryCodeKey = "CountryCode";
extern string const kUniqueIdKey = "UniqueId";
extern string const kLanguageKey = "Language";

namespace
{
NSDictionary * const kDeviceNamesBeforeMetalDriver = @{
  @"i386" : @"Simulator",
  @"iPad1,1" : @"iPad WiFi",
  @"iPad1,2" : @"iPad GSM",
  @"iPad2,1" : @"iPad 2 WiFi",
  @"iPad2,2" : @"iPad 2 CDMA",
  @"iPad2,2" : @"iPad 2 GSM",
  @"iPad2,3" : @"iPad 2 GSM EV-DO",
  @"iPad2,4" : @"iPad 2",
  @"iPad2,5" : @"iPad Mini WiFi",
  @"iPad2,6" : @"iPad Mini GSM",
  @"iPad2,7" : @"iPad Mini CDMA",
  @"iPad3,1" : @"iPad 3rd gen. WiFi",
  @"iPad3,2" : @"iPad 3rd gen. GSM",
  @"iPad3,3" : @"iPad 3rd gen. CDMA",
  @"iPad3,4" : @"iPad 4th gen. WiFi",
  @"iPad3,5" : @"iPad 4th gen. GSM",
  @"iPad3,6" : @"iPad 4th gen. CDMA",
  @"iPad4,1" : @"iPad Air WiFi",
  @"iPad4,2" : @"iPad Air GSM",
  @"iPad4,3" : @"iPad Air CDMA",
  @"iPad4,4" : @"iPad Mini 2nd gen. WiFi",
  @"iPad4,5" : @"iPad Mini 2nd gen. GSM",
  @"iPad4,6" : @"iPad Mini 2nd gen. CDMA",
  @"iPad5,3" : @"iPad Air 2 WiFi",
  @"iPad5,4" : @"iPad Air 2 GSM",
  @"iPhone1,1" : @"iPhone",
  @"iPhone1,2" : @"iPhone 3G",
  @"iPhone2,1" : @"iPhone 3GS",
  @"iPhone3,1" : @"iPhone 4 GSM",
  @"iPhone3,2" : @"iPhone 4 CDMA",
  @"iPhone3,3" : @"iPhone 4 GSM EV-DO",
  @"iPhone4,1" : @"iPhone 4S",
  @"iPhone4,2" : @"iPhone 4S",
  @"iPhone4,3" : @"iPhone 4S",
  @"iPhone5,1" : @"iPhone 5",
  @"iPhone5,2" : @"iPhone 5",
  @"iPhone5,3" : @"iPhone 5c",
  @"iPhone5,4" : @"iPhone 5c",
  @"iPhone6,1" : @"iPhone 5s",
  @"iPhone6,2" : @"iPhone 5s",
  @"iPhone7,1" : @"iPhone 6 Plus",
  @"iPhone7,2" : @"iPhone 6",
  @"iPod1,1" : @"iPod Touch",
  @"iPod2,1" : @"iPod Touch 2nd gen.",
  @"iPod3,1" : @"iPod Touch 3rd gen.",
  @"iPod4,1" : @"iPod Touch 4th gen.",
  @"iPod5,1" : @"iPod Touch 5th gen.",
  @"x86_64" : @"Simulator"
};

NSDictionary * const kDeviceNamesWithiOS10MetalDriver = @{
  @"iPad6,3" : @"iPad Pro (9.7 inch) WiFi",
  @"iPad6,4" : @"iPad Pro (9.7 inch) GSM",
  @"iPad6,7" : @"iPad Pro (12.9 inch) WiFi",
  @"iPad6,8" : @"iPad Pro (12.9 inch) GSM",
  @"iPhone8,1" : @"iPhone 6s",
  @"iPhone8,2" : @"iPhone 6s Plus",
  @"iPhone8,4" : @"iPhone SE"
};
NSDictionary * const kDeviceNamesWithMetalDriver = @{
  @"iPhone9,1" : @"iPhone 7",
  @"iPhone9,2" : @"iPhone 7 Plus",
  @"iPhone9,3" : @"iPhone 7",
  @"iPhone9,4" : @"iPhone 7 Plus"
};

}  // namespace

@interface AppInfo ()

@property(nonatomic) NSString * countryCode;
@property(nonatomic) NSString * uniqueId;
@property(nonatomic) NSString * bundleVersion;
@property(nonatomic) NSString * buildNumber;
@property(nonatomic) NSString * deviceInfo;
@property(nonatomic) NSUUID * advertisingId;
@property(nonatomic) NSDate * buildDate;
@property(nonatomic) NSString * deviceName;

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

- (NSString *)countryCode
{
  if (!_countryCode)
  {
    CTTelephonyNetworkInfo * networkInfo = [[CTTelephonyNetworkInfo alloc] init];
    CTCarrier * carrier = networkInfo.subscriberCellularProvider;
    if ([carrier.isoCountryCode length])  // if device can access sim card info
      _countryCode = [carrier.isoCountryCode uppercaseString];
    else  // else, getting system country code
      _countryCode = [[NSLocale.currentLocale objectForKey:NSLocaleCountryCode] uppercaseString];

    std::string codeString;
    if (settings::Get(kCountryCodeKey, codeString))  // if country code stored in settings
    {
      if (carrier.isoCountryCode)  // if device can access sim card info
        settings::Set(kCountryCodeKey,
                      std::string(_countryCode.UTF8String));  // then save new code instead
      else
        _countryCode =
            @(codeString.c_str());  // if device can NOT access sim card info then using saved code
    }
    else
    {
      if (_countryCode)
        settings::Set(kCountryCodeKey,
                      std::string(_countryCode.UTF8String));  // saving code first time
      else
        _countryCode = @"";
    }
  }
  return _countryCode;
}

- (NSString *)uniqueId
{
  if (!_uniqueId)
  {
    string uniqueString;
    if (settings::Get(kUniqueIdKey, uniqueString))  // if id stored in settings
    {
      _uniqueId = @(uniqueString.c_str());
    }
    else  // if id not stored in settings
    {
      _uniqueId = [UIDevice.currentDevice.identifierForVendor UUIDString];
      if (_uniqueId)  // then saving in settings
        settings::Set(kUniqueIdKey, std::string(_uniqueId.UTF8String));
    }
  }
  return _uniqueId;
}

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

- (NSUUID *)advertisingId
{
  if (!_advertisingId)
  {
    ASIdentifierManager * m = [ASIdentifierManager sharedManager];
    if (m.isAdvertisingTrackingEnabled)
      _advertisingId = m.advertisingIdentifier;
  }
  return _advertisingId;
}

- (NSString *)languageId
{
  NSArray * languages = NSLocale.preferredLanguages;
  return languages.count == 0 ? nil : languages[0];
}

- (NSString *)twoLetterLanguageId
{
  NSString * languageId = self.languageId;
  auto constexpr maxCodeLength = 2UL;
  auto const length = languageId.length;
  if (length > maxCodeLength)
    languageId = [languageId substringToIndex:maxCodeLength];
  else if (length < maxCodeLength)
    languageId = @"en";

  return languageId;
}

- (NSDate *)buildDate
{
  if (!_buildDate)
  {
    NSString * dateStr = [NSString stringWithFormat:@"%@ %@", @(__DATE__), @(__TIME__)];
    NSDateFormatter * dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"LLL d yyyy HH:mm:ss"];
    [dateFormatter setLocale:[[NSLocale alloc] initWithLocaleIdentifier:@"en_US_POSIX"]];
    _buildDate = [dateFormatter dateFromString:dateStr];
  }
  return _buildDate;
}

- (NSString *)deviceName
{
  if (!_deviceName)
  {
    struct utsname systemInfo;
    uname(&systemInfo);
    NSString * machine = @(systemInfo.machine);
    _deviceName = kDeviceNamesBeforeMetalDriver[machine];
    if (!_deviceName)
      _deviceName = kDeviceNamesWithiOS10MetalDriver[machine];
    if (!_deviceName)
      _deviceName = kDeviceNamesWithMetalDriver[machine];
    else
      _deviceName = machine;
  }
  return _deviceName;
}

- (MWMOpenGLDriver)openGLDriver
{
  struct utsname systemInfo;
  uname(&systemInfo);
  NSString * machine = @(systemInfo.machine);
  if (kDeviceNamesBeforeMetalDriver[machine] != nil)
    return MWMOpenGLDriverRegular;
  if (kDeviceNamesWithiOS10MetalDriver[machine] != nil)
  {
    if (isIOSVersionLessThan(10))
      return MWMOpenGLDriverRegular;
    else if (isIOSVersionLessThan(@"10.3"))
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
  NSString * networkCode =
      [[CTTelephonyNetworkInfo alloc] init].subscriberCellularProvider.mobileNetworkCode;
  return networkCode != nil && networkCode.length > 0 && ![networkCode isEqualToString:@"65535"];
}

@end
