
#import "AppInfo.h"
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <sys/utsname.h>
#import <AdSupport/ASIdentifierManager.h>
#include "../../../platform/settings.hpp"

NSString * const AppFeatureInterstitial = @"AppFeatureInterstitial";
NSString * const AppFeatureBanner = @"AppFeatureBanner";
NSString * const AppFeatureProButtonOnMap = @"AppFeatureProButtonOnMap";
NSString * const AppFeatureMoreAppsBanner = @"AppFeatureMoreAppsBanner";
NSString * const AppFeatureBottomMenuItems = @"AppFeatureBottomMenuItems";

NSString * const AppInfoSyncedNotification = @"AppInfoSyncedNotification";

@interface AppInfo ()

@property (nonatomic) NSDictionary * features;
@property (nonatomic) NSDictionary * featuresByDefault;

@property (nonatomic) NSString * countryCode;
@property (nonatomic) NSString * uniqueId;
@property (nonatomic) Reachability * reachability;
@property (nonatomic) NSInteger launchCount;
@property (nonatomic) NSDate * firstLaunchDate;

@end

@implementation AppInfo

- (void)dealloc
{
  [self.reachability stopNotifier];
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
  self.launchCount++;
}

- (void)update
{
  NSString * urlString = @"http://application.server/ios/features_v2.json";
  NSURLRequest * request = [NSURLRequest requestWithURL:[NSURL URLWithString:urlString] cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:20];
  [NSURLConnection sendAsynchronousRequest:request queue:[NSOperationQueue mainQueue] completionHandler:^(NSURLResponse * r, NSData * d, NSError * e){
    if ([(NSHTTPURLResponse *)r statusCode] == 200)
    {
      [d writeToFile:[self featuresPath] atomically:YES];
      _features = nil;
      [self.reachability stopNotifier];
      [[NSNotificationCenter defaultCenter] postNotificationName:AppInfoSyncedNotification object:nil];
    }
    else
    {
      __weak id weakSelf = self;
      self.reachability.reachableBlock = ^(Reachability * r){
        if ([r isReachable])
          [weakSelf update];
      };
      [self.reachability startNotifier];
    }
  }];
}

- (NSString *)featuresPath
{
  NSString * libraryPath = [NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES) firstObject];
  return [libraryPath stringByAppendingPathComponent:@"AvailableFeatures.json"];
}

#pragma mark - Public methods

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

  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];

  self.featuresByDefault = @{AppFeatureInterstitial : @NO,
                             AppFeatureBanner : @NO,
                             AppFeatureProButtonOnMap : @YES,
                             AppFeatureMoreAppsBanner : @YES};

  [self update];

  if (!self.firstLaunchDate)
    self.firstLaunchDate = [NSDate date];

  return self;
}

- (BOOL)featureAvailable:(NSString *)featureName
{
  if (!self.features) // features haven't been downloaded yet
    return [self.featuresByDefault[featureName] boolValue];

  return [self featureParameters:featureName] != nil;
}

- (id)featureValue:(NSString *)featureName forKey:(NSString *)key
{
  NSDictionary *parameters = [self featureParameters:featureName];
  if (parameters && parameters[key])
    return parameters[key];

  return nil;
}

- (NSDictionary *)findValueForKey:(NSString *)key inDictionary:(NSDictionary *)parameters
{
  for (NSString * aKey in [parameters allKeys])
    if ([aKey rangeOfString:key].location != NSNotFound)
      return parameters[aKey];
  return nil;
}

- (NSDictionary *)featureParameters:(NSString *)featureName
{
  NSDictionary * parameters = [self findValueForKey:[NSString stringWithFormat:@"Country=%@", self.countryCode] inDictionary:self.features[featureName]];
  if (!parameters)
    parameters = [self findValueForKey:[NSString stringWithFormat:@"Lang=%@", [[NSLocale preferredLanguages] firstObject]] inDictionary:self.features[featureName]];

  if (!parameters)
    parameters = [self findValueForKey:@"*" inDictionary:self.features[featureName]];

  return parameters;
}

- (NSString *)snapshot
{
  return [NSString stringWithFormat:@"maps.me ver. %@, %@ (iOS %@) %@", self.bundleVersion, self.deviceInfo, self.firmwareVersion, self.countryCode];
}

#pragma mark - Public properties

- (void)setLaunchCount:(NSInteger)launchCount
{
  Settings::Set("LaunchCount", (int)launchCount);
}

- (NSInteger)launchCount
{
  int count = 0;
  Settings::Get("LaunchCount", count);
  return count;
}

- (NSString *)uniqueId
{
  if (!_uniqueId)
  {
    string uniqueString;
    if (Settings::Get("UniqueId", uniqueString)) // if id stored in settings
    {
      _uniqueId = [NSString stringWithUTF8String:uniqueString.c_str()];
    }
    else // if id not stored in settings
    {
      _uniqueId = [[UIDevice currentDevice].identifierForVendor UUIDString];
      if (_uniqueId) // then saving in settings
        Settings::Set("UniqueId", std::string([_uniqueId UTF8String]));
    }
  }
  return _uniqueId;
}

- (NSUUID *)advertisingId
{
  if (NSClassFromString(@"ASIdentifierManager"))
  {
    ASIdentifierManager * m = [ASIdentifierManager sharedManager];
    if (m.isAdvertisingTrackingEnabled)
      return m.advertisingIdentifier;
  }

  return nil;
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
    if (Settings::Get("CountryCode", codeString)) // if country code stored in settings
    {
      if (carrier.isoCountryCode) // if device can access sim card info
        Settings::Set("CountryCode", std::string([_countryCode UTF8String])); // then save new code instead
      else
        _countryCode = [NSString stringWithUTF8String:codeString.c_str()]; // if device can NOT access sim card info then using saved code
    }
    else
    {
      if (_countryCode)
        Settings::Set("CountryCode", std::string([_countryCode UTF8String])); // saving code first time
      else
        _countryCode = @"";
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

- (NSDate *)firstLaunchDate
{
  return [[NSUserDefaults standardUserDefaults] objectForKey:@"AppInfoFirstLaunchDate"];
}

- (void)setFirstLaunchDate:(NSDate *)firstLaunchDate
{
  [[NSUserDefaults standardUserDefaults] setObject:firstLaunchDate forKey:@"AppInfoFirstLaunchDate"];
  [[NSUserDefaults standardUserDefaults] synchronize];
}

#pragma mark - Private properties

- (NSDictionary *)features
{
  if (!_features)
  {
    NSData * data = [NSData dataWithContentsOfFile:[self featuresPath]];
    if (data)
      _features = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:nil];
  }
  return _features;
}

@end
