#include "platform/utils.hpp"

#import <Foundation/NSUserDefaults.h>

namespace platform
{
bool IsGuidesLayerFirstLaunch()
{
  NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;
  return ![ud boolForKey:@"guidesWasShown"];
}

void SetGuidesLayerFirstLaunch(bool isFirstLaunch)
{
  NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:isFirstLaunch forKey:@"guidesWasShown"];
  [ud synchronize];
}
}  // namespace platform
