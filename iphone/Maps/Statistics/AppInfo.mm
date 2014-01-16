
#import "AppInfo.h"
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <sys/utsname.h>
#import <AdSupport/ASIdentifierManager.h>
#include "../../../platform/settings.hpp"

NSString * const AppFeatureInterstitialAd = @"InterstitialAd";
NSString * const AppFeatureBannerAd = @"BannerAd";

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

@end

@implementation AppInfo

- (instancetype)init
{
  self = [super init];

  self.featuresByDefault = @{AppFeatureInterstitialAd : @NO,
                             AppFeatureBannerAd : @NO};

  [self update];

  return self;
}

- (NSDictionary *)parsedFeatures:(NSString *)featuresString
{
  NSArray * features = [featuresString componentsSeparatedByString:@";"];
  NSMutableDictionary * mDict = [NSMutableDictionary dictionary];
  for (NSString * featurePairString in features)
  {
    NSArray * components = [featurePairString componentsSeparatedByString:@":"];
    if ([components count] == 2)
      mDict[components[0]] = components[1];
  }
  return mDict;
}

- (void)update
{
  NSString * urlString = @"http://application.server/ios/features.txt";
  NSURLRequest * request = [NSURLRequest requestWithURL:[NSURL URLWithString:urlString]];
  [NSURLConnection sendAsynchronousRequest:request queue:[NSOperationQueue mainQueue] completionHandler:^(NSURLResponse * r, NSData * d, NSError * e){
    if ([(NSHTTPURLResponse *)r statusCode] == 200)
    {
      NSString * featuresString = [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding];
      featuresString = [featuresString stringByReplacingOccurrencesOfString:@" " withString:@""];
      std::string featuresCPPString;
      if (!Settings::Get("AvailableFeatures", featuresCPPString))
        self.features = [self parsedFeatures:featuresString];
      Settings::Set("AvailableFeatures", std::string([featuresString UTF8String]));
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

#pragma mark - Public methods

- (BOOL)featureAvailable:(NSString *)featureName
{
  if (!self.features) // features haven't been downloaded yet
    return [self.featuresByDefault[featureName] boolValue];

  NSString * value = self.features[featureName];
  if (value)
  {
    NSString * countryCode = [@"@" stringByAppendingString:self.countryCode];
    if ([value rangeOfString:countryCode options:NSCaseInsensitiveSearch].location != NSNotFound)
      return YES;
    if ([value rangeOfString:@"*"].location != NSNotFound)
      return YES;
  }
  return NO;
}

- (NSString *)featureValue:(NSString *)featureName forKey:(NSString *)key
{
  NSString * value = self.features[featureName];
  NSArray * components = [value componentsSeparatedByString:@"@"];
  for (NSString * countryComponent in components)
  {
    BOOL countryExist = [countryComponent rangeOfString:self.countryCode options:NSCaseInsensitiveSearch].location != NSNotFound;
    BOOL allExist = [countryComponent rangeOfString:@"*" options:NSCaseInsensitiveSearch].location != NSNotFound;
    if (countryExist || allExist)
    {
      NSArray * parameters = [countryComponent componentsSeparatedByString:@"#"];
      for (NSString * parameter in parameters) {
        NSRange range = [parameter rangeOfString:key options:NSCaseInsensitiveSearch];
        NSInteger startIndex = range.location + range.length + 1;
        if (range.location != NSNotFound && [parameter length] > startIndex)
          return [parameter substringFromIndex:startIndex];
      }
    }
  }
  return nil;
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

#pragma mark - Private properties

- (NSDictionary *)features
{
  if (!_features)
  {
    std::string featuresCPPString;
    if (Settings::Get("AvailableFeatures", featuresCPPString))
    {
      NSString * featuresString = [NSString stringWithUTF8String:featuresCPPString.c_str()];
      _features = [self parsedFeatures:featuresString];
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
