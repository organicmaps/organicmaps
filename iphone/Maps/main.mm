#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <Foundation/NSThread.h>

#include "../../platform/settings.hpp"
#include "../../platform/platform.hpp"

#include "../../base/logging.hpp"


/// Used to trick iOs and enable multithreading support with non-native pthreads.
@interface Dummy : NSObject 
- (void) dummyThread: (id) obj;
@end

@implementation Dummy
- (void) dummyThread: (id) obj {}
@end


/// Used to log NSExceptions.
static void exceptionHandler(NSException * ex)
{
  NSArray *stack = [ex callStackSymbols];
  NSLog(@"%@", ex.name);
  NSLog(@"%@", ex.reason);
  NSLog(@"Stack trace: %@", stack);
}

int main(int argc, char * argv[])
{
  LOG(LINFO, ("maps.me started, detected CPU cores:", GetPlatform().CpuCores()));

  NSSetUncaughtExceptionHandler(&exceptionHandler);
  int retVal;
  @autoreleasepool
  {
    Dummy * dummy = [Dummy alloc];
    NSThread * thread = [[NSThread alloc] initWithTarget:dummy selector:@selector(dummyThread:) object:nil];
    [thread start];

    // We use this work-around to avoid multiple calls for Settings::IsFirstLaunch() later with invalid results,
    // as it returns true only once, on the very first call
    [[NSUserDefaults standardUserDefaults] setBool:(Settings::IsFirstLaunch()) forKey:FIRST_LAUNCH_KEY];
    [[NSUserDefaults standardUserDefaults] synchronize];

    retVal = UIApplicationMain(argc, argv, nil, nil);
  }
  return retVal;
}
