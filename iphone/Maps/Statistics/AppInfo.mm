
#import "AppInfo.h"
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <sys/utsname.h>
#import <AdSupport/ASIdentifierManager.h>
#include "../../../platform/settings.hpp"

NSString * const AppFeatureMoPubInterstitial = @"AppFeatureMoPubInterstitial";
NSString * const AppFeatureMWMProInterstitial = @"AppFeatureMWMProInterstitial";
NSString * const AppFeatureMoPubBanner = @"AppFeatureMoPubBanner";
NSString * const AppFeatureMWMProBanner = @"AppFeatureMWMProBanner";

@interface AppInfo ()

@property (nonatomic) NSDictionary * features;
@property (nonatomic) NSDictionary * featuresByDefault;

@property (nonatomic) NSString * countryCode;
@property (nonatomic) NSString * bundleVersion;
@property (nonatomic) NSString * deviceInfo;
@property (nonatomic) NSString * firmwareVersion;
@property (nonatomic) NSString * uniqueId;
@property (nonatomic) NSString * advertisingId;
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
  NSString * urlString = @"http://application.server/ios/features.json";
  NSURLRequest * request = [NSURLRequest requestWithURL:[NSURL URLWithString:urlString]];
  [NSURLConnection sendAsynchronousRequest:request queue:[NSOperationQueue mainQueue] completionHandler:^(NSURLResponse * r, NSData * d, NSError * e){
    if ([(NSHTTPURLResponse *)r statusCode] == 200)
    {
      [d writeToFile:[self featuresPath] atomically:YES];
      [self.reachability stopNotifier];
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

  self.featuresByDefault = @{AppFeatureMoPubInterstitial : @NO,
                             AppFeatureMWMProInterstitial : @NO,
                             AppFeatureMoPubBanner : @NO,
                             AppFeatureMWMProBanner : @NO};

  [self update];

  if (!self.firstLaunchDate)
    self.firstLaunchDate = [NSDate date];

  return self;
}

- (void)setup {}

- (BOOL)featureAvailable:(NSString *)featureName
{
  if (!self.features) // features haven't been downloaded yet
    return [self.featuresByDefault[featureName] boolValue];

  NSDictionary * localizations = self.features[featureName];
  return localizations[self.countryCode] || localizations[@"*"];
}

- (id)featureValue:(NSString *)featureName forKey:(NSString *)key defaultValue:(id)defaultValue
{
  NSDictionary * localizations = self.features[featureName];
  NSDictionary * parameters = localizations[self.countryCode];
  if (!parameters)
    parameters = localizations[@"*"];
  if (parameters && parameters[key])
    return parameters[key];

  return defaultValue;
}

- (NSString *)snapshot
{
  return [NSString stringWithFormat:@"MapsWithMe ver. %@, %@ (iOS %@) %@", self.bundleVersion, self.deviceInfo, self.firmwareVersion, self.countryCode];
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
    if ([carrier.isoCountryCode length]) // if device can access sim card info
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
