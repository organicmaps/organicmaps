#import "AppInfo.h"
#import "MWMCustomFacebookEvents.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"
#import "Flurry.h"
#import <MyTrackerSDKCorp/MRMyTracker.h>
#import <MyTrackerSDKCorp/MRTrackerParams.h>
#import <FBSDKCoreKit/FBSDKCoreKit.h>

#include "platform/settings.hpp"

// If you have a "missing header error" here, then please run configure.sh script in the root repo folder.
#import "../../../private.h"

char const * kStatisticsEnabledSettingsKey = "StatisticsEnabled";

@interface Statistics ()
{
  bool _enabled;
}
@property (nonatomic) NSDate * lastLocationLogTimestamp;
@end

@implementation Statistics

+ (bool)isStatisticsEnabledByDefault
{
#ifdef OMIM_PRODUCTION
  return true;
#else
  // Make developer's life a little bit easier.
  [Alohalytics setDebugMode:YES];
  return false;
#endif
}

- (instancetype)init
{
  if ((self = [super init]))
  {
    _enabled = [Statistics isStatisticsEnabledByDefault];
    // Note by AlexZ:
    // _enabled should be persistent across app's process lifecycle. That's why we change
    // _enabled property only once - when the app is launched. In this case we don't need additional
    // checks and specific initializations for different 3party engines, code is much cleaner and safer
    // (actually, we don't have a choice - 3party SDKs do not guarantee correctness if not initialized
    // in application:didFinishLaunchingWithOptions:).
    // The (only) drawback of this approach is that to actually disable or enable 3party engines,
    // the app should be restarted.
    (void)Settings::Get(kStatisticsEnabledSettingsKey, _enabled);

    if (_enabled)
      [Alohalytics enable];
    else
      [Alohalytics disable];
  }
  return self;
}

- (void)enableOnNextAppLaunch
{
  // This setting will be checked and applied on the next launch.
  Settings::Set(kStatisticsEnabledSettingsKey, true);
  // It does not make sense to log statisticsEnabled with Alohalytics here,
  // as it will not be stored and logged anyway.
}

- (void)disableOnNextAppLaunch
{
  // This setting will be checked and applied on the next launch.
  Settings::Set(kStatisticsEnabledSettingsKey, false);
  [Alohalytics logEvent:@"statisticsDisabled"];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  // _enabled should be already correctly set up in init method.
  if (_enabled)
  {
    [Flurry startSession:@(FLURRY_KEY)];
    [Flurry logAllPageViewsForTarget:application.windows.firstObject.rootViewController];

    [MRMyTracker createTracker:@(MY_TRACKER_KEY)];
#ifdef DEBUG
    [MRMyTracker setDebugMode:YES];
#endif
    [MRMyTracker getTrackerParams].trackAppLaunch = YES;
    [MRMyTracker setupTracker];

    [Alohalytics setup:@(ALOHALYTICS_URL) withLaunchOptions:launchOptions];
  }
  // Always call Facebook method, looks like it is required to handle some url schemes and sign on scenarios.
  return [[FBSDKApplicationDelegate sharedInstance] application:application didFinishLaunchingWithOptions:launchOptions];
}

- (void)logLocation:(CLLocation *)location
{
  if (!_enabled)
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
  if (!_enabled)
    return;
  NSMutableDictionary * params = [parameters mutableCopy];
  params[kStatDeviceType] = IPAD ? kStatiPad : kStatiPhone;
  BOOL isLandscape = UIDeviceOrientationIsLandscape([UIDevice currentDevice].orientation);
  params[kStatOrientation] = isLandscape ? kStatLandscape : kStatPortrait;
  AppInfo * info = [AppInfo sharedInfo];
  params[kStatCountry] = info.countryCode;
  if (info.languageId)
    params[kStatLanguage] = info.languageId;
  [Flurry logEvent:eventName withParameters:parameters];
  [Alohalytics logEvent:eventName withDictionary:parameters];
}

- (void)logEvent:(NSString *)eventName
{
  [self logEvent:eventName withParameters:@{}];
}

- (void)logApiUsage:(NSString *)programName
{
  if (!_enabled)
    return;
  if (programName)
    [self logEvent:@"Api Usage" withParameters: @{@"Application Name" : programName}];
  else
    [self logEvent:@"Api Usage" withParameters: @{@"Application Name" : @"Error passing nil as SourceApp name."}];
}

- (void)applicationDidBecomeActive
{
  if (!_enabled)
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
    instance = [[Statistics alloc] init];
  });
  return instance;
}

@end