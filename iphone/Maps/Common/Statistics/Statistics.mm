#import "Statistics.h"
#import "Statistics+ConnectionTypeLogging.h"
#import "AppInfo.h"
#import "MWMCustomFacebookEvents.h"
#import "MWMSettings.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"
#import "Flurry.h"
#import <MyTrackerSDK/MRMyTracker.h>
#import <MyTrackerSDK/MRMyTrackerParams.h>
#import <FBSDKCoreKit/FBSDKCoreKit.h>

#include "platform/settings.hpp"

#include "base/macros.hpp"

// If you have a "missing header error" here, then please run configure.sh script in the root repo folder.
#import "private.h"

namespace
{
void checkFlurryLogStatus(FlurryEventRecordStatus status)
{
  NSCAssert(status == FlurryEventRecorded || status == FlurryEventLoggingDelayed,
            @"Flurry log event failed.");
}
}  // namespace

@interface Statistics ()

@property(nonatomic) NSDate * lastLocationLogTimestamp;

@end

@implementation Statistics

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  // _enabled should be already correctly set up in init method.
  if ([MWMSettings statisticsEnabled])
  {
    auto sessionBuilder = [[[FlurrySessionBuilder alloc] init]
                           withAppVersion:[AppInfo sharedInfo].bundleVersion];
    [Flurry startSession:@(FLURRY_KEY) withSessionBuilder:sessionBuilder];
    [Flurry logAllPageViewsForTarget:application.windows.firstObject.rootViewController];

    [MRMyTracker createTracker:@(MY_TRACKER_KEY)];
#ifdef DEBUG
    [MRMyTracker setDebugMode:YES];
#endif
    [MRMyTracker trackerParams].trackLaunch = YES;
    [MRMyTracker setupTracker];

    [Alohalytics setup:@(ALOHALYTICS_URL) withLaunchOptions:launchOptions];
  }
  // Always call Facebook method, looks like it is required to handle some url schemes and sign on scenarios.
  return [[FBSDKApplicationDelegate sharedInstance] application:application didFinishLaunchingWithOptions:launchOptions];
}

- (void)logLocation:(CLLocation *)location
{
  if (![MWMSettings statisticsEnabled])
    return;
  if (!_lastLocationLogTimestamp || [[NSDate date] timeIntervalSinceDate:_lastLocationLogTimestamp] > (60 * 60 * 3))
  {
    _lastLocationLogTimestamp = [NSDate date];
    CLLocationCoordinate2D const coord = location.coordinate;
    [Flurry setLatitude:coord.latitude longitude:coord.longitude horizontalAccuracy:location.horizontalAccuracy verticalAccuracy:location.verticalAccuracy];
  }
}

- (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters
{
  if (![MWMSettings statisticsEnabled])
    return;
  NSMutableDictionary * params = [self addDefaultAttributesToParameters:parameters];
  [Alohalytics logEvent:eventName withDictionary:params];
  checkFlurryLogStatus([Flurry logEvent:eventName withParameters:params]);
}

- (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters atLocation:(CLLocation *)location
{
  if (![MWMSettings statisticsEnabled])
    return;
  NSMutableDictionary * params = [self addDefaultAttributesToParameters:parameters];
  [Alohalytics logEvent:eventName withDictionary:params atLocation:location];
  auto const & coordinate = location ? location.coordinate : kCLLocationCoordinate2DInvalid;
  params[kStatLocation] = makeLocationEventValue(coordinate.latitude, coordinate.longitude);
  checkFlurryLogStatus([Flurry logEvent:eventName withParameters:params]);
}

- (NSMutableDictionary *)addDefaultAttributesToParameters:(NSDictionary *)parameters
{
  NSMutableDictionary * params = [parameters mutableCopy];
  BOOL isLandscape = UIDeviceOrientationIsLandscape(UIDevice.currentDevice.orientation);
  params[kStatOrientation] = isLandscape ? kStatLandscape : kStatPortrait;
  AppInfo * info = [AppInfo sharedInfo];
  params[kStatCountry] = info.countryCode;
  if (info.languageId)
    params[kStatLanguage] = info.languageId;
  return params;
}

- (void)logEvent:(NSString *)eventName
{
  [self logEvent:eventName withParameters:@{}];
}

- (void)logApiUsage:(NSString *)programName
{
  if (![MWMSettings statisticsEnabled])
    return;
  if (programName)
    [self logEvent:@"Api Usage" withParameters: @{@"Application Name" : programName}];
  else
    [self logEvent:@"Api Usage" withParameters: @{@"Application Name" : @"Error passing nil as SourceApp name."}];
}

- (void)applicationDidBecomeActive
{
  if (![MWMSettings statisticsEnabled])
    return;
  [FBSDKAppEvents activateApp];
  // Special FB events to improve marketing campaigns quality.
  [MWMCustomFacebookEvents optimizeExpenses];
}

+ (instancetype)instance
{
  static Statistics * instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^
  {
    instance = [[self alloc] init];
  });
  return instance;
}

+ (void)logEvent:(NSString *)eventName
{
  [[self instance] logEvent:eventName];
}

+ (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters
{
  [[self instance] logEvent:eventName withParameters:parameters];
}

+ (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters atLocation:(CLLocation *)location
{
  [[self instance] logEvent:eventName withParameters:parameters atLocation:location];
}

@end

@implementation Statistics (ConnectionTypeLogging)

+ (NSString *)connectionTypeToString:(Platform::EConnectionType)type
{
  switch (type)
  {
  case Platform::EConnectionType::CONNECTION_WWAN:
    return kStatOffline;
  case Platform::EConnectionType::CONNECTION_WIFI:
    return kStatWifi;
  case Platform::EConnectionType::CONNECTION_NONE:
    return kStatNone;
  }
}

@end
