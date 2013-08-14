#import "Statistics.h"

#import "Flurry.h"

@implementation Statistics

- (void) startSession
{
  [Flurry startSession:
      [[NSBundle mainBundle] objectForInfoDictionaryKey:@"FlurryKey"]];
  [self configure];
}

- (void) stopSession
{
}

- (void) configure
{
  [Flurry setCrashReportingEnabled:YES];
  [Flurry setSessionReportsOnPauseEnabled:NO];
}

- (void) logEvent:(NSString *)eventName
{
  [Flurry logEvent:eventName];
}

- (void) logEvent:(NSString *)eventName withParameters:(NSDictionary *)parameters
{
  [Flurry logEvent:eventName withParameters:parameters];
}

- (void)logProposalReason:(NSString *)reason withAnswer:(NSString *)answer
{
  NSString * screen = [NSString stringWithFormat:@"Open AppStore With Proposal on %@", reason];
  [[Statistics instance] logEvent:screen withParameters:@{@"Answer" : answer}];
}

- (void)logApiUsage:(NSString *)programName
{
  if (programName)
    [[Statistics instance] logEvent:@"Api Usage" withParameters: @{@"Application Name" : programName}];
  else
    [[Statistics instance] logEvent:@"Api Usage" withParameters: @{@"Application Name" : @"Error passing nil as SourceApp name."}];
}

+ (Statistics *) instance
{
  static Statistics* instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[Statistics alloc] init];
    
    // Init.
    [Flurry setSecureTransportEnabled:true];
  });
  return instance;
}

@end