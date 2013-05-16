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
  NSDictionary * dict = [NSDictionary dictionaryWithObjects:[NSArray arrayWithObjects: answer, nil]
                                                    forKeys:[NSArray arrayWithObjects: @"Answer", nil]];
  NSString * screen = [NSString stringWithFormat:@"Open AppStore With Proposal on %@", reason];
  [[Statistics instance] logEvent:screen withParameters:dict];
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