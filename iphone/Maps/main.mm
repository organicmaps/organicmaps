#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <Foundation/NSThread.h>
#import "Common.h"

#include "../../platform/file_logging.hpp"
#include "../../platform/platform.hpp"
#include "../../platform/settings.hpp"

int main(int argc, char * argv[])
{
#ifdef MWM_LOG_TO_FILE
  my::SetLogMessageFn(LogMessageFile);
#endif
  LOG(LINFO, ("maps.me started, detected CPU cores:", GetPlatform().CpuCores()));

  int retVal;
  @autoreleasepool
  {
    retVal = UIApplicationMain(argc, argv, nil, nil);
  }
  return retVal;
}
