#import <Pushwoosh/PushNotificationManager.h>
#import "MapsAppDelegate.h"

#ifdef OMIM_PRODUCTION
#import <AppsFlyerLib/AppsFlyerTracker.h>
#include "fabric_logging.hpp"
#endif

#include "platform/file_logging.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

void setPushWooshSender()
{
  GetPlatform().GetMarketingService().SetPushWooshSender([](std::string const & tag, std::vector<std::string> const & values)
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
  GetPlatform().GetMarketingService().SetMarketingSender([](std::string const & tag, std::map<std::string, std::string> const & params)
  {
    if (tag.empty())
      return;
    NSMutableDictionary<NSString *, NSString *> * eventParams = [@{} mutableCopy];
    for (auto const & param : params)
    {
      NSString * key = @(param.first.c_str());
      NSString * value = @(param.second.c_str());
      eventParams[key] = value;
    }

  #ifdef OMIM_PRODUCTION
    [[AppsFlyerTracker sharedTracker] trackEvent:@(tag.c_str()) withValues:eventParams];
  #endif
  });
}

int main(int argc, char * argv[])
{
#ifdef MWM_LOG_TO_FILE
  base::SetLogMessageFn(LogMessageFile);
#elif OMIM_PRODUCTION
  base::SetLogMessageFn(platform::IosLogMessage);
  base::SetAssertFunction(platform::IosAssertMessage);
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
