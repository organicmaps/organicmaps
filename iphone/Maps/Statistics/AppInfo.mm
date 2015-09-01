
#import "AppInfo.h"
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <sys/utsname.h>
#import <AdSupport/ASIdentifierManager.h>

#include "platform/settings.hpp"

extern string const kCountryCodeKey = "CountryCode";
extern string const kUniqueIdKey = "UniqueId";
extern string const kLanguageKey = "Language";

static string const kLaunchCountKey = "LaunchCount";
static NSString * const kAppInfoFirstLaunchDateKey = @"AppInfoFirstLaunchDate";

@interface AppInfo()

@property (nonatomic) NSString * snapshot;
@property (nonatomic) NSString * countryCode;
@property (nonatomic) NSString * uniqueId;
@property (nonatomic) NSInteger launchCount;
@property (nonatomic) NSDate * firstLaunchDate;
@property (nonatomic) NSString * bundleVersion;
@property (nonatomic) NSString * deviceInfo;
@property (nonatomic) NSUUID * advertisingId;

@end

@implementation AppInfo

+ (instancetype)sharedInfo
{
  static AppInfo * appInfo;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^
  {
    appInfo = [[self alloc] init];
  });
  return appInfo;
}

- (instancetype)init
{
  self = [super init];
  [self setup];
  return self;
}

- (void)setup
{
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];

  if (!self.firstLaunchDate)
    self.firstLaunchDate = [NSDate date];
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
  self.launchCount++;
}

#pragma mark - Properties

- (NSString *)snapshot
{
  if (!_snapshot)
    _snapshot = [NSString stringWithFormat:@"maps.me ver. %@, %@ (iOS %@) %@", self.bundleVersion, self.deviceInfo, [UIDevice currentDevice].systemVersion, self.countryCode];
  return _snapshot;
}

- (NSString *)countryCode
{
  if (!_countryCode)
  {
    CTTelephonyNetworkInfo * networkInfo = [[CTTelephonyNetworkInfo alloc] init];
    CTCarrier * carrier = networkInfo.subscriberCellularProvider;
    if ([carrier.isoCountryCode length]) // if device can access sim card info
      _countryCode = [carrier.isoCountryCode uppercaseString];
    else // else, getting system country code
      _countryCode = [[[NSLocale currentLocale] objectForKey:NSLocaleCountryCode] uppercaseString];

    std::string codeString;
    if (Settings::Get(kCountryCodeKey, codeString)) // if country code stored in settings
    {
      if (carrier.isoCountryCode) // if device can access sim card info
        Settings::Set(kCountryCodeKey, std::string([_countryCode UTF8String])); // then save new code instead
      else
        _countryCode = @(codeString.c_str()); // if device can NOT access sim card info then using saved code
    }
    else
    {
      if (_countryCode)
        Settings::Set(kCountryCodeKey, std::string([_countryCode UTF8String])); // saving code first time
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
    if (Settings::Get(kUniqueIdKey, uniqueString)) // if id stored in settings
    {
      _uniqueId = @(uniqueString.c_str());
    }
    else // if id not stored in settings
    {
      _uniqueId = [[UIDevice currentDevice].identifierForVendor UUIDString];
      if (_uniqueId) // then saving in settings
        Settings::Set(kUniqueIdKey, std::string([_uniqueId UTF8String]));
    }
  }
  return _uniqueId;
}

- (void)setLaunchCount:(NSInteger)launchCount
{
  Settings::Set(kLaunchCountKey, (int)launchCount);
}

- (NSInteger)launchCount
{
  int count = 0;
  Settings::Get(kLaunchCountKey, count);
  return count;
}

- (void)setFirstLaunchDate:(NSDate *)firstLaunchDate
{
  [[NSUserDefaults standardUserDefaults] setObject:firstLaunchDate forKey:kAppInfoFirstLaunchDateKey];
  [[NSUserDefaults standardUserDefaults] synchronize];
}

- (NSDate *)firstLaunchDate
{
  return [[NSUserDefaults standardUserDefaults] objectForKey:kAppInfoFirstLaunchDateKey];
}

- (NSString *)bundleVersion
{
  if (!_bundleVersion)
    _bundleVersion = [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"];
  return _bundleVersion;
}

- (NSString *)deviceInfo
{
  if (!_deviceInfo)
  {
    struct utsname systemInfo;
    uname(&systemInfo);
    _deviceInfo = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
  }
  return _deviceInfo;
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
  NSArray * languages = [NSLocale preferredLanguages];
  return languages.count == 0 ? nil : languages[0];
}

@end
