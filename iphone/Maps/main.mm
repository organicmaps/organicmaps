#import "MapsAppDelegate.h"

#include "platform/file_logging.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"


int main(int argc, char * argv[])
{
#ifdef MWM_LOG_TO_FILE
  base::SetLogMessageFn(LogMessageFile);
#endif
  auto & p = GetPlatform();
  LOG(LINFO, ("omaps started, detected CPU cores:", p.CpuCores()));

  int retVal;
  @autoreleasepool
  {
    retVal = UIApplicationMain(argc, argv, nil, NSStringFromClass([MapsAppDelegate class]));
  }
  return retVal;
}
