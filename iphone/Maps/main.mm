#import "Common.h"
#import "MapsAppDelegate.h"

#include "platform/file_logging.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

int main(int argc, char * argv[])
{
#ifdef MWM_LOG_TO_FILE
  my::SetLogMessageFn(LogMessageFile);
#endif
  LOG(LINFO, ("maps.me started, detected CPU cores:", GetPlatform().CpuCores()));

  int retVal;
  @autoreleasepool
  {
    retVal = UIApplicationMain(argc, argv, nil, NSStringFromClass([MapsAppDelegate class]));
  }
  return retVal;
}
