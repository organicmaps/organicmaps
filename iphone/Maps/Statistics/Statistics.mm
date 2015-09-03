#import "Statistics.h"
#import "Flurry.h"
#import "AppInfo.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "platform/settings.hpp"

static constexpr char const * kStatisticsEnabledSettingsKey = "StatisticsEnabled";

@interface Statistics ()
@property (nonatomic) NSDate * lastLocationLogTimestamp;
@end

@implementation Statistics

- (void)startSessionWithLaunchOptions:(NSDictionary *)launchOptions
{
  if (self.enabled)
  {
    [Flurry startSession:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"FlurryKey"]];
  }
}

- (void)logLocation:(CLLocation *)location
{
  if (self.enabled)
  {
    if (!_lastLocationLogTimestamp || [[NSDate date] timeIntervalSinceDate:_lastLocationLogTimestamp] > (60 * 60 * 3))
    {
      _lastLocationLogTimestamp = [NSDate date];
      CLLocationCoordinate2D const coord = location.coordinate;
      [Flurry setLatitude:coord.latitude longitude:coord.longitude horizontalAccuracy:location.horizontalAccuracy verticalAccuracy:location.verticalAccuracy];
    }
  }
}

- (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters
{
  if (self.enabled)
    [Flurry logEvent:eventName withParameters:parameters];
}

- (void)logEvent:(NSString *)eventName
{
  [self logEvent:eventName withParameters:nil];
}

- (void)logApiUsage:(NSString *)programName
{
  if (self.enabled)
  {
    if (programName)
      [self logEvent:@"Api Usage" withParameters: @{@"Application Name" : programName}];
    else
      [self logEvent:@"Api Usage" withParameters: @{@"Application Name" : @"Error passing nil as SourceApp name."}];
  }
}

- (BOOL)enabled
{
#ifdef DEBUG
  bool statisticsEnabled = false;
#else
  bool statisticsEnabled = true;
#endif
  (void)Settings::Get(kStatisticsEnabledSettingsKey, statisticsEnabled);
  return statisticsEnabled;
}

- (void)setEnabled:(BOOL)enabled
{
  Settings::Set(kStatisticsEnabledSettingsKey, static_cast<bool>(enabled));
  if (enabled)
    [Alohalytics enable];
  else
    [Alohalytics disable];
}

+ (instancetype)instance
{
  static Statistics * instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[Statistics alloc] init];
  });
  return instance;
}

@end