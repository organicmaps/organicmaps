#import "Statistics.h"
#include "../../../platform/settings.hpp"
#import "Flurry.h"

@implementation Statistics

- (void)startSession
{
  if (self.enabled)
  {
    [Flurry startSession:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"FlurryKey"]];
    [self configure];
  }
}

- (void)configure
{
  if (self.enabled)
  {
    [Flurry setCrashReportingEnabled:YES];
    [Flurry setSessionReportsOnPauseEnabled:NO];
  }
}

- (void)logEvent:(NSString *)eventName
{
  if (self.enabled)
    [Flurry logEvent:eventName];
}

- (void)logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters
{
  if (self.enabled)
    [Flurry logEvent:eventName withParameters:parameters];
}

- (void)logProposalReason:(NSString *)reason withAnswer:(NSString *)answer
{
  if (self.enabled)
  {
    NSString * screen = [NSString stringWithFormat:@"Open AppStore With Proposal on %@", reason];
    [[Statistics instance] logEvent:screen withParameters:@{@"Answer" : answer}];
  }
}

- (void)logApiUsage:(NSString *)programName
{
  if (self.enabled)
  {
    if (programName)
      [[Statistics instance] logEvent:@"Api Usage" withParameters: @{@"Application Name" : programName}];
    else
      [[Statistics instance] logEvent:@"Api Usage" withParameters: @{@"Application Name" : @"Error passing nil as SourceApp name."}];
  }
}

- (id)init
{
  self = [super init];

  if (self.enabled)
    [Flurry setSecureTransportEnabled:true];

  return self;
}

- (BOOL)enabled
{
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