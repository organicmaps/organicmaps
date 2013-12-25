
#import "AppInfo.h"
#import "Reachability.h"
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <sys/utsname.h>
#import <AdSupport/ASIdentifierManager.h>
#include "../../../platform/settings.hpp"

@interface AppInfo ()

@property Reachability * reachability;
@property (nonatomic) NSArray * features;
@property NSDictionary * featuresByDefault;

@end

@implementation AppInfo

- (instancetype)init
{
  self = [super init];

  // example featuresByDefault
  self.featuresByDefault = @{@"ads" : @NO, @"popup" : @YES};
  //

  [self update];

  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidEnterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];

  return self;
}

- (void)applicationDidEnterBackground:(NSNotification *)notification
{
  NSString * featuresString = @"";
  for (NSDictionary * featureDict in self.features) {
    if (![featureDict count])
      continue;
    if ([featuresString length])
      featuresString = [featuresString stringByAppendingString:@";"];
    featuresString = [NSString stringWithFormat:@"%@{%@:%@}", featuresString, [featureDict allKeys][0], [featureDict allValues][0]];
  }
  Settings::Set("AvailableFeatures", std::string([featuresString UTF8String]));
}

- (void)update
{
  // TODO: get info from server
  // if not success then subscribe for reachability updates
  self.reachability = [Reachability reachabilityForInternetConnection];
  __weak id weakSelf = self;
  self.reachability.reachableBlock = ^(Reachability * r){
    [weakSelf update];
  };
  [self.reachability startNotifier];
}

- (BOOL)featureAvailable:(NSString *)featureName
{
  if (!self.features) // features haven't been downloaded yet
    return [self.featuresByDefault[featureName] boolValue];

  for (NSDictionary * feature in self.features)
  {
    if ([[[feature allKeys] firstObject] isEqualToString:featureName])
      return ([feature[@"locals"] rangeOfString:self.countryCode].location != NSNotFound) || [feature[@"locals"] isEqualToString:@"*"];
  }
  return NO;
}

#pragma mark - Public Methods

- (NSString *)snapshot
{
  return [NSString stringWithFormat:@"MapsWithMe ver. %@, %@ (iOS %@) %@", self.bundleVersion, self.deviceInfo, self.firmwareVersion, self.countryCode];
}

#pragma mark - Public properties

- (NSString *)userId
{
  if (NSClassFromString(@"ASIdentifierManager"))
    return [[ASIdentifierManager sharedManager].advertisingIdentifier UUIDString];
  else
    return nil;
}

- (NSString *)countryCode
{
  if (!_countryCode)
  {
    CTTelephonyNetworkInfo * networkInfo = [[CTTelephonyNetworkInfo alloc] init];
    CTCarrier * carrier = networkInfo.subscriberCellularProvider;
    _countryCode = [carrier.isoCountryCode uppercaseString];
    if (!_countryCode)
      _countryCode = [[NSLocale currentLocale] objectForKey:NSLocaleCountryCode];
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

#pragma mark - Private properties

- (NSArray *)features
{
  if (!_features)
  {
    std::string featuresCPPString;
    if (Settings::Get("AvailableFeatures", featuresCPPString))
    {
      NSString * featuresString = [NSString stringWithUTF8String:featuresCPPString.c_str()];
      NSArray * features = [featuresString componentsSeparatedByString:@";"];
      NSMutableArray * mArray = [NSMutableArray array];
      for (NSString * featureDictString in features) {
        NSString * trimmed = [featureDictString stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"{}"]];
        NSArray * components = [trimmed componentsSeparatedByString:@":"];
        if ([components count] == 2)
          [mArray addObject:@{components[0] : components[1]}];
      }
      _features = mArray;
    }
  }
  return _features;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
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
