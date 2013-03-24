#import "Statistics.h"

#import "Flurry.h"

@implementation Statistics

- (void) startSession
{
  NSLog(@"Stats: Start session.");
  [Flurry startSession:
      [[NSBundle mainBundle] objectForInfoDictionaryKey:@"FlurryKey"]];
}

- (void) stopSession
{
  NSLog(@"Stats: Stop session.");
}

+ (Statistics *) instance
{
  static Statistics *instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[Statistics alloc] init];
    
    // Init.
    [Flurry setSecureTransportEnabled:true];
  });
  return instance;
}

@end