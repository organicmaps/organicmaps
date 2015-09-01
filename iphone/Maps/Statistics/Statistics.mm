#import "Statistics.h"
#import "Flurry.h"
#import "AppInfo.h"

#include "platform/settings.hpp"

@implementation Statistics

- (void)startSessionWithLaunchOptions:(NSDictionary *)launchOptions
{
  if (self.enabled)
  {
    [Flurry startSession:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"FlurryKey"]];
    [Flurry setSessionReportsOnPauseEnabled:NO];
  }
}

- (void)logLocation:(CLLocation *)location
{
  if (self.enabled)
  {
    static NSDate * lastUpdate;
    if (!lastUpdate || [[NSDate date] timeIntervalSinceDate:lastUpdate] > (60 * 60 * 3))
    {
      lastUpdate = [NSDate date];
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

- (void)logInAppMessageEvent:(NSString *)eventName imageType:(NSString *)imageType
{
  NSString * language = [[NSLocale preferredLanguages] firstObject];
  AppInfo * info = [AppInfo sharedInfo];
  [self logEvent:eventName withParameters:@{@"Type": imageType, @"Country" : info.countryCode, @"Language" : language, @"Id" : info.uniqueId}];
}

- (void)logApiUsage:(NSString *)programName
{
  if (programName)
    [self logEvent:@"Api Usage" withParameters: @{@"Application Name" : programName}];
  else
    [self logEvent:@"Api Usage" withParameters: @{@"Application Name" : @"Error passing nil as SourceApp name."}];
}

- (BOOL)enabled
{
#ifdef DEBUG
  return NO;
#else
  bool statisticsEnabled = true;
  Settings::Get("StatisticsEnabled", statisticsEnabled);

  return statisticsEnabled;
#endif
}

+ (Statistics *)instance
{
  static Statistics * instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[Statistics alloc] init];
  });
  return instance;
}

@end