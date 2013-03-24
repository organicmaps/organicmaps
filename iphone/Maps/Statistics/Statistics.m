#import "Statistics.h"

#import "Flurry.h"

@implementation Statistics

- (void) startSession
{
  [Flurry startSession:
      [[NSBundle mainBundle] objectForInfoDictionaryKey:@"FlurryKey"]];
}

- (void) stopSession
{
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