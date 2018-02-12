#import "MWMNetworkPolicy.h"
#import "MWMAlertViewController.h"

#include "platform/platform.hpp"

namespace
{
NSString * const kNetworkingPolicyTimeStamp = @"NetworkingPolicyTimeStamp";
NSString * const kNetworkingPolicyStage = @"NetworkingPolicyStage";
NSTimeInterval const kSessionDurationSeconds = 24 * 60 * 60;
}  // namespace

namespace network_policy
{
void SetStage(Stage stage)
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setInteger:static_cast<NSInteger>(stage) forKey:kNetworkingPolicyStage];
  [ud setObject:[NSDate dateWithTimeIntervalSinceNow:kSessionDurationSeconds]
         forKey:kNetworkingPolicyTimeStamp];
}

Stage const GetStage()
{
  return static_cast<Stage>(
      [NSUserDefaults.standardUserDefaults integerForKey:kNetworkingPolicyStage]);
}

NSDate * GetPolicyDate()
{
  return [NSUserDefaults.standardUserDefaults objectForKey:kNetworkingPolicyTimeStamp];
}

void CallPartnersApi(platform::PartnersApiFn fn, bool force)
{
  auto const connectionType = GetPlatform().ConnectionStatus();
  if (connectionType == Platform::EConnectionType::CONNECTION_NONE)
  {
    fn(false);
    return;
  }
  if (force || connectionType == Platform::EConnectionType::CONNECTION_WIFI)
  {
    fn(true);
    return;
  }

  auto checkAndApply = ^bool {
    switch (GetStage())
    {
    case Stage::Ask: return false;
    case Stage::Always: fn(true); return true;
    case Stage::Never: fn(false); return true;
    case Stage::Today:
      if ([GetPolicyDate() compare:[NSDate date]] == NSOrderedDescending)
      {
        fn(true);
        return true;
      }
      return false;
    case Stage::NotToday:
      if ([GetPolicyDate() compare:[NSDate date]] == NSOrderedDescending)
      {
        fn(false);
        return true;
      }
      return false;
    }
  };

  if (checkAndApply())
    return;

  dispatch_async(dispatch_get_main_queue(), ^{
    MWMAlertViewController * alertController = [MWMAlertViewController activeAlertController];
    [alertController presentMobileInternetAlertWithBlock:^{
      if (!checkAndApply())
        fn(false);
    }];
  });
}

bool CanUseNetwork()
{
  using ct = Platform::EConnectionType;
  switch (GetPlatform().ConnectionStatus())
  {
  case ct::CONNECTION_NONE: return false;
  case ct::CONNECTION_WIFI: return true;
  case ct::CONNECTION_WWAN:
    switch (GetStage())
    {
    case Stage::Ask: return false;
    case Stage::Always: return true;
    case Stage::Never: return false;
    case Stage::Today: return [GetPolicyDate() compare:[NSDate date]] == NSOrderedDescending;
    case Stage::NotToday: return false;
    }
  }
}
}  // namespace network_policy
