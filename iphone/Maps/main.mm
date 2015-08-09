#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <Foundation/NSThread.h>
#import "Common.h"

#include "../../platform/file_logging.hpp"
#include "../../platform/platform.hpp"
#include "../../platform/settings.hpp"


/// Used to trick iOs and enable multithreading support with non-native pthreads.
@interface Dummy : NSObject 
- (void) dummyThread: (id) obj;
@end

@implementation Dummy
- (void) dummyThread: (id) obj {}
@end

int main(int argc, char * argv[])
{
#ifdef MWM_LOG_TO_FILE
  my::SetLogMessageFn(LogMessageFile);
#endif
  LOG(LINFO, ("maps.me started, detected CPU cores:", GetPlatform().CpuCores()));

  int retVal;
  @autoreleasepool
  {
    Dummy * dummy = [Dummy alloc];
    NSThread * thread = [[NSThread alloc] initWithTarget:dummy selector:@selector(dummyThread:) object:nil];
    [thread start];

    retVal = UIApplicationMain(argc, argv, nil, nil);
  }
  return retVal;
}
