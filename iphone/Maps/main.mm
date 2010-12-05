#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <Foundation/NSThread.h>

#include "../../platform/platform.hpp"
#include "../../base/logging.hpp"

//////////////////////////////////////////////////////////////////////////////////
/// Used to trick iOs and enable multithreading support with non-native pthreads
@interface Dummy : NSObject
{
}
- (void) dummyThread: (id) obj;
@end

@implementation Dummy
- (void) dummyThread: (id) obj
{
}
@end
///////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{
  LOG(LINFO, ("MapsWithMe started, detected CPU cores:", GetPlatform().CpuCores()));

  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

  Dummy * dummy = [[Dummy alloc] autorelease];
  NSThread * thread = [[NSThread alloc] initWithTarget:dummy selector:@selector(dummyThread:) object:nil];
  [thread start];

  int retVal = UIApplicationMain(argc, argv, nil, nil);
  [pool release];
  return retVal;
}
