#import "Statistics.h"
#import "MWMCustomFacebookEvents.h"
#import "MWMSettings.h"

#import "3party/Alohalytics/src/alohalytics.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"
#import "Flurry.h"
#import <FBSDKCoreKit/FBSDKCoreKit.h>
#import <AdSupport/ASIdentifierManager.h>
#import <CoreApi/AppInfo.h>

#include "platform/platform.hpp"

// If you have a "missing header error" here, then please run configure.sh script in the root repo folder.
#import "private.h"

namespace
{
void checkFlurryLogStatus(FlurryEventRecordStatus status) {
  NSCAssert(status == FlurryEventRecorded || status == FlurryEventLoggingDelayed,
            @"Flurry log event failed.");
}
  
NSInteger convertToAlohalyticsChannel(StatisticsChannel cnannel) {
  switch (cnannel) {
    case StatisticsChannelDefault: return (NSInteger)alohalytics::ChannelMask(0);
    case StatisticsChannelRealtime: return (NSInteger)(alohalytics::ChannelMask(0) | alohalytics::ChannelMask(1));
  }
  return (NSInteger)alohalytics::ChannelMask(0);
}
}  // namespace

@interface Statistics ()

@property(nonatomic) NSDate * lastLocationLogTimestamp;

@end

@implementation Statistics

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  // _enabled should be already correctly set up in init method.
  if ([MWMSettings statisticsEnabled]) {
    if ([ASIdentifierManager sharedManager].advertisingTrackingEnabled) {
      auto sessionBuilder = [[[FlurrySessionBuilder alloc] init]
                             withAppVersion:[AppInfo sharedInfo].bundleVersion];
      [sessionBuilder withDataSaleOptOut:true];
      [Flurry startSession:@(FLURRY_KEY) withSessionBuilder:sessionBuilder];
    }

    [Alohalytics setup:@[@(ALOHALYTICS_URL), [NSString stringWithFormat:@"%@/%@", @(ALOHALYTICS_URL), @"realtime"]]
     withLaunchOptions:launchOptions];
  }
  // Always call Facebook method, looks like it is required to handle some url schemes and sign on scenarios.
  return [[FBSDKApplicationDelegate sharedInstance] application:application didFinishLaunchingWithOptions:launchOptions];
}

- (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters
     withChannel:(StatisticsChannel)channel {
  if (![MWMSettings statisticsEnabled])
    return;
  NSMutableDictionary * params = [self addDefaultAttributesToParameters:parameters];
  [Alohalytics logEvent:eventName withDictionary:params withChannel:convertToAlohalyticsChannel(channel)];
  dispatch_async(dispatch_get_main_queue(), ^{
    checkFlurryLogStatus([Flurry logEvent:eventName withParameters:params]);
  });
}

- (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters atLocation:(CLLocation *)location
     withChannel:(StatisticsChannel)channel {
  if (![MWMSettings statisticsEnabled])
    return;
  NSMutableDictionary * params = [self addDefaultAttributesToParameters:parameters];
  [Alohalytics logEvent:eventName withDictionary:params atLocation:location
            withChannel:convertToAlohalyticsChannel(channel)];
  auto const & coordinate = location ? location.coordinate : kCLLocationCoordinate2DInvalid;
  params[kStatLocation] = makeLocationEventValue(coordinate.latitude, coordinate.longitude);
  dispatch_async(dispatch_get_main_queue(), ^{
    checkFlurryLogStatus([Flurry logEvent:eventName withParameters:params]);
  });
}

- (NSMutableDictionary *)addDefaultAttributesToParameters:(NSDictionary *)parameters {
  NSMutableDictionary * params = [parameters mutableCopy];
  BOOL isLandscape = UIDeviceOrientationIsLandscape(UIDevice.currentDevice.orientation);
  params[kStatOrientation] = isLandscape ? kStatLandscape : kStatPortrait;
  AppInfo * info = [AppInfo sharedInfo];
  params[kStatCountry] = info.countryCode;
  if (info.languageId)
    params[kStatLanguage] = info.languageId;
  return params;
}

- (void)logEvent:(NSString *)eventName withChannel:(StatisticsChannel)channel {
  [self logEvent:eventName withParameters:@{} withChannel:channel];
}

- (void)logApiUsage:(NSString *)programName {
  if (![MWMSettings statisticsEnabled])
    return;
  if (programName) {
    [self logEvent:@"Api Usage" withParameters: @{@"Application Name" : programName}
       withChannel:StatisticsChannelDefault];
  } else {
    [self logEvent:@"Api Usage" withParameters: @{@"Application Name" : @"Error passing nil as SourceApp name."}
       withChannel:StatisticsChannelDefault];
  }
}

- (void)applicationDidBecomeActive {
  if (![MWMSettings statisticsEnabled])
    return;
  [FBSDKAppEvents activateApp];
  // Special FB events to improve marketing campaigns quality.
  [MWMCustomFacebookEvents optimizeExpenses];
}

+ (instancetype)instance {
  static Statistics * instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[self alloc] init];
  });
  return instance;
}

+ (void)logEvent:(NSString *)eventName {
  [[self instance] logEvent:eventName withChannel:StatisticsChannelDefault];
}

+ (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters {
  [[self instance] logEvent:eventName withParameters:parameters withChannel:StatisticsChannelDefault];
}

+ (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters atLocation:(CLLocation *)location {
  [[self instance] logEvent:eventName withParameters:parameters atLocation:location
                withChannel:StatisticsChannelDefault];
}

+ (void)logEvent:(NSString *)eventName withChannel:(StatisticsChannel)channel {
   [[self instance] logEvent:eventName withChannel:channel];
}

+ (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters
     withChannel:(StatisticsChannel)channel {
  [[self instance] logEvent:eventName withParameters:parameters withChannel:channel];
}

+ (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters atLocation:(CLLocation *)location
     withChannel:(StatisticsChannel)channel {
  [[self instance] logEvent:eventName withParameters:parameters atLocation:location withChannel:channel];
}

+ (NSString *)connectionTypeString {
  switch (Platform::ConnectionStatus()) {
  case Platform::EConnectionType::CONNECTION_WWAN:
    return kStatMobile;
  case Platform::EConnectionType::CONNECTION_WIFI:
    return kStatWifi;
  case Platform::EConnectionType::CONNECTION_NONE:
    return kStatNone;
  }
}

@end
