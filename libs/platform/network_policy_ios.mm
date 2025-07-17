#include "platform/network_policy_ios.h"

#include "platform/platform.hpp"

#import <Foundation/NSDate.h>
#import <Foundation/NSString.h>
#import <Foundation/NSUserDefaults.h>

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
  NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;
  [ud setInteger:stage forKey:kNetworkingPolicyStage];
  [ud setObject:[NSDate dateWithTimeIntervalSinceNow:kSessionDurationSeconds] forKey:kNetworkingPolicyTimeStamp];
}

Stage GetStage()
{
  return (Stage)[NSUserDefaults.standardUserDefaults integerForKey:kNetworkingPolicyStage];
}

NSDate * GetPolicyDate()
{
  return [NSUserDefaults.standardUserDefaults objectForKey:kNetworkingPolicyTimeStamp];
}

bool IsActivePolicyDate()
{
  return [GetPolicyDate() compare:[NSDate date]] == NSOrderedDescending;
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
    case Stage::Today: return IsActivePolicyDate();
    case Stage::NotToday: return false;
    }
  }
}
}  // namespace network_policy

namespace platform
{
NetworkPolicy GetCurrentNetworkPolicy()
{
  return NetworkPolicy(network_policy::CanUseNetwork());
}
}  // namespace platform
