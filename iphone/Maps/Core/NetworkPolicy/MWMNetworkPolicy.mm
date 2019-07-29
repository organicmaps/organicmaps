#import "MWMNetworkPolicy.h"
#import "MWMAlertViewController.h"

#include "platform/platform.hpp"

namespace network_policy
{
void CallPartnersApi(platform::PartnersApiFn fn, bool force, bool showAnyway)
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
    case Stage::Never:
        if (showAnyway) {
          return false;
        }
        fn(false);
        return true;
    case Stage::Today:
      if (IsActivePolicyDate())
      {
        fn(true);
        return true;
      }
      return false;
    case Stage::NotToday:
      if (showAnyway) {
        return false;
      }
      if (IsActivePolicyDate())
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
}  // namespace network_policy
