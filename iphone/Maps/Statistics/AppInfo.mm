
#import "AppInfo.h"
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <sys/utsname.h>
#import <AdSupport/ASIdentifierManager.h>
#include "../../../platform/settings.hpp"

@interface AppInfo ()

@property (nonatomic) NSDictionary * features;
@property NSDictionary * featuresByDefault;

@end

@implementation AppInfo

- (instancetype)init
{
  self = [super init];

  [self update];

  return self;
}

- (void)saveFeaturesOnDisk
{
  // example of features string @"ads:ru,en;banners:*;track:jp,en"
  NSString * featuresString = @"";
  for (NSString * featureName in [self.features allKeys])
  {
    if ([featuresString length])
      featuresString = [featuresString stringByAppendingString:@";"];
    featuresString = [NSString stringWithFormat:@"%@%@:%@", featuresString, featureName, self.features[featureName]];
  }
  Settings::Set("AvailableFeatures", std::string([featuresString UTF8String]));
}

- (void)update
{
  // TODO: get info from server
  // if done then saving features on disk
  [self saveFeaturesOnDisk];

  // if failed then subscribing for reachability updates
  __weak id weakSelf = self;
  self.reachability.reachableBlock = ^(Reachability * r){
    [weakSelf update];
  };
  [self.reachability startNotifier];
}

#pragma mark - Public methods

- (BOOL)featureAvailable:(NSString *)featureName
{
  if (!self.features) // features haven't been downloaded yet
    return [self.featuresByDefault[featureName] boolValue];

  NSString * value = self.features[featureName];
  if (value)
    return ([value rangeOfString:self.countryCode].location != NSNotFound) || [value isEqualToString:@"*"];
  return NO;
}

- (NSString *)snapshot
{
  return [NSString stringWithFormat:@"MapsWithMe ver. %@, %@ (iOS %@) %@", self.bundleVersion, self.deviceInfo, self.firmwareVersion, self.countryCode];
}

#pragma mark - Public properties

- (NSString *)uniqueId
{
  if (!_uniqueId)
  {
    std::string uniqueString;
    if (Settings::Get("UniqueId", uniqueString)) // if id stored in settings
    {
      _uniqueId = [NSString stringWithUTF8String:uniqueString.c_str()];
    }
    else // if id not stored in settings
    {
      // trying to get id
      if ([UIDevice instancesRespondToSelector:@selector(identifierForVendor)])
        _uniqueId = [[UIDevice currentDevice].identifierForVendor UUIDString];
      else
        _uniqueId = (NSString *)CFBridgingRelease(CFUUIDCreateString(kCFAllocatorDefault, CFUUIDCreate(kCFAllocatorDefault)));

      if (_uniqueId) // then saving in settings
        Settings::Set("UniqueId", std::string([_uniqueId UTF8String]));
    }
  }
  return _uniqueId;
}

- (NSString *)advertisingId
{
  return NSClassFromString(@"ASIdentifierManager") ? [[ASIdentifierManager sharedManager].advertisingIdentifier UUIDString] : nil;
}

- (NSString *)countryCode
{
  if (!_countryCode)
  {
    CTTelephonyNetworkInfo * networkInfo = [[CTTelephonyNetworkInfo alloc] init];
    CTCarrier * carrier = networkInfo.subscriberCellularProvider;
    if (carrier.isoCountryCode) // if device can access sim card info
      _countryCode = [carrier.isoCountryCode uppercaseString];
    else // else, getting system country code
      _countryCode = [[NSLocale currentLocale] objectForKey:NSLocaleCountryCode];

    std::string codeString;
    if (Settings::Get("CountryCode", codeString)) // if country code stored in settings
    {
      if (carrier.isoCountryCode) // if device can access sim card info
        Settings::Set("CountryCode", std::string([_countryCode UTF8String])); // then save new code instead
      else
        _countryCode = [NSString stringWithUTF8String:codeString.c_str()]; // if device can NOT access sim card info then using saved code
    }
    else
    {
      Settings::Set("CountryCode", std::string([_countryCode UTF8String])); // saving code first time
    }
  }
  return _countryCode;
}

- (NSString *)bundleVersion
{
  return [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"];
}

- (NSString *)deviceInfo
{
  struct utsname systemInfo;
  uname(&systemInfo);
  return [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
}

- (NSString *)firmwareVersion
{
  return [UIDevice currentDevice].systemVersion;
}

- (Reachability *)reachability
{
  if (!_reachability)
    _reachability = [Reachability reachabilityForInternetConnection];
  return _reachability;
}

#pragma mark - Private properties

- (NSDictionary *)features
{
  if (!_features)
  {
    std::string featuresCPPString;
    if (Settings::Get("AvailableFeatures", featuresCPPString))
    {
      NSString * featuresString = [NSString stringWithUTF8String:featuresCPPString.c_str()];
      NSArray * features = [featuresString componentsSeparatedByString:@";"];
      NSMutableDictionary * mDict = [NSMutableDictionary dictionary];
      for (NSString * featurePairString in features)
      {
        NSArray * components = [featurePairString componentsSeparatedByString:@":"];
        if ([components count] == 2)
          mDict[components[0]] = components[1];
      }
      _features = mDict;
    }
  }
  return _features;
}

- (void)dealloc
{
  [self.reachability stopNotifier];
}

+ (instancetype)sharedInfo
{
  static AppInfo * appInfo;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    appInfo = [[self alloc] init];
  });
  return appInfo;
}

@end
