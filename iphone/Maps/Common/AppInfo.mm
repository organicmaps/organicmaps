#import "AppInfo.h"
#import "MWMCommon.h"
#import "SwiftBridge.h"

#include "platform/platform_ios.h"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include <sys/utsname.h>

#import <AdSupport/ASIdentifierManager.h>
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>

namespace
{
string const kCountryCodeKey = "CountryCode";
string const kUniqueIdKey = "UniqueId";
}  // namespace

@interface AppInfo ()

@property(nonatomic) NSString * countryCode;
@property(nonatomic) NSString * uniqueId;
@property(nonatomic) NSString * bundleVersion;
@property(nonatomic) NSString * buildNumber;
@property(nonatomic) NSString * deviceInfo;
@property(nonatomic) NSUUID * advertisingId;
@property(nonatomic) NSDate * buildDate;
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

- (NSString *)inputLanguage
{
  auto window = UIApplication.sharedApplication.keyWindow;
  auto firstResponder = [window firstResponder];
  if (!firstResponder)
    return self.languageId;
  auto textInputMode = firstResponder.textInputMode;
  if (!textInputMode)
    return self.languageId;
  return textInputMode.primaryLanguage;
}

- (NSString *)twoLetterInputLanguage
{
  return @(languages::Normalize(self.inputLanguage.UTF8String).c_str());
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

- (NSString *)deviceModel
{
  if (!_deviceModel)
    _deviceModel = @(GetPlatform().DeviceModel().c_str());
  return _deviceModel;
}

- (MWMOpenGLDriver)openGLDriver
{
  struct utsname systemInfo;
  uname(&systemInfo);
  NSString * machine = @(systemInfo.machine);
  if (platform::kDeviceModelsBeforeMetalDriver[machine] != nil)
    return MWMOpenGLDriverRegular;
  if (platform::kDeviceModelsWithiOS10MetalDriver[machine] != nil)
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
