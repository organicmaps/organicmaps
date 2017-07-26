#import <MyTrackerSDK/MRMyTracker.h>
#import <Pushwoosh/PushNotificationManager.h>
#import "MWMCommon.h"
#import "MapsAppDelegate.h"

#ifdef OMIM_PRODUCTION
# include "fabric_logging.hpp"
#endif

#include "platform/file_logging.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

void setPushWooshSender()
{
  GetPlatform().GetMarketingService().SetPushWooshSender([](string const & tag, vector<string> const & values)
  {
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
}

void setMarketingSender()
{
  GetPlatform().GetMarketingService().SetMarketingSender([](string const & tag, map<string, string> const & params)
  {
    if (tag.empty())
      return;
    NSMutableDictionary<NSString *, NSString *> * eventParams = [@{} mutableCopy];
    NSMutableString * myTrackerEvent = [@(tag.c_str()) mutableCopy];
    for (auto const & param : params)
    {
      NSString * key = @(param.first.c_str());
      NSString * value = @(param.second.c_str());
      eventParams[key] = value;
      [myTrackerEvent appendString:[NSString stringWithFormat:@"_%@_%@", key, value]];
    }
    [MRMyTracker trackEventWithName:myTrackerEvent];
  });
}

int main(int argc, char * argv[])
{
#ifdef MWM_LOG_TO_FILE
  my::SetLogMessageFn(LogMessageFile);
#elif OMIM_PRODUCTION
  my::SetLogMessageFn(platform::IosLogMessage);
  my::SetAssertFunction(platform::IosAssertMessage);
#endif
  auto & p = GetPlatform();
  LOG(LINFO, ("maps.me started, detected CPU cores:", p.CpuCores()));

  setPushWooshSender();
  setMarketingSender();

  int retVal;
  @autoreleasepool
  {
    retVal = UIApplicationMain(argc, argv, nil, NSStringFromClass([MapsAppDelegate class]));
  }
  return retVal;
}
