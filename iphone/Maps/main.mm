#import <Pushwoosh/PushNotificationManager.h>
#import "Common.h"
#import "MapsAppDelegate.h"

#ifdef OMIM_PRODUCTION
# include "fabric_logging.hpp"
#endif

#include "platform/file_logging.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

int main(int argc, char * argv[])
{
#ifdef MWM_LOG_TO_FILE
  my::SetLogMessageFn(LogMessageFile);
#elif OMIM_PRODUCTION
  my::SetLogMessageFn(platform::LogMessageFabric);
#endif
  auto & p = GetPlatform();
  LOG(LINFO, ("maps.me started, detected CPU cores:", p.CpuCores()));

  p.SetPushWooshSender([](string const & tag, vector<string> const & values) {
    if (values.empty() || tag.empty())
      return;
    PushNotificationManager * pushManager = [PushNotificationManager pushManager];
    if (values.size() == 1)
    {
      [pushManager setTags:@{ @(tag.c_str()) : @(values.front().c_str()) }];
    }
    else
    {
      NSMutableArray<NSString *> * tags = [@[] mutableCopy];
      for (auto const & value : values)
        [tags addObject:@(value.c_str())];
      [pushManager setTags:@{ @(tag.c_str()) : tags }];
    }
  });

  int retVal;
  @autoreleasepool
  {
    retVal = UIApplicationMain(argc, argv, nil, NSStringFromClass([MapsAppDelegate class]));
  }
  return retVal;
}
