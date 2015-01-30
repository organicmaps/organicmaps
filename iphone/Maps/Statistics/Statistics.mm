
#import "Statistics.h"
#include "../../../platform/settings.hpp"
#import "Flurry.h"
#import "AppInfo.h"

@implementation Statistics

- (void)startSessionWithLaunchOptions:(NSDictionary *)launchOptions
{
  if (self.enabled)
  {
    [Flurry startSession:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"FlurryKey"]];
    [Flurry setCrashReportingEnabled:YES];
    [Flurry setSessionReportsOnPauseEnabled:NO];
  }
}

- (void)logLatitude:(double)latitude longitude:(double)longitude horizontalAccuracy:(double)horizontalAccuracy verticalAccuracy:(double)verticalAccuracy
{
  if (self.enabled)
  {
    static NSDate * lastUpdate;
    if (!lastUpdate || [[NSDate date] timeIntervalSinceDate:lastUpdate] > (60 * 60 * 3))
    {
      lastUpdate = [NSDate date];
      [Flurry setLatitude:latitude longitude:longitude horizontalAccuracy:horizontalAccuracy verticalAccuracy:verticalAccuracy];
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

- (void)applicationWillResignActive
{
}

- (void)applicationWillTerminate
{
}

- (void)applicationDidEnterBackground
{
}

- (void)applicationWillEnterForeground
{
}

- (void)applicationDidBecomeActive
{
}

- (BOOL)enabled
{
#ifdef DEBUG
  return NO;
#endif

  bool statisticsEnabled = true;
  Settings::Get("StatisticsEnabled", statisticsEnabled);

  return statisticsEnabled;
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