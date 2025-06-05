#import "MWMNetworkPolicy.h"

#include "platform/network_policy_ios.h"
#include "platform/platform.hpp"

@implementation MWMNetworkPolicy

+ (MWMNetworkPolicy *)sharedPolicy
{
  static MWMNetworkPolicy * policy;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ policy = [[MWMNetworkPolicy alloc] init]; });
  return policy;
}

- (MWMNetworkPolicyPermission)permission
{
  switch (network_policy::GetStage())
  {
  case network_policy::Ask: return MWMNetworkPolicyPermissionAsk;
  case network_policy::Always: return MWMNetworkPolicyPermissionAlways;
  case network_policy::Never: return MWMNetworkPolicyPermissionNever;
  case network_policy::Today: return MWMNetworkPolicyPermissionToday;
  case network_policy::NotToday: return MWMNetworkPolicyPermissionNotToday;
  }
}

- (void)setPermission:(MWMNetworkPolicyPermission)permission
{
  network_policy::Stage stage;
  switch (permission)
  {
  case MWMNetworkPolicyPermissionAsk: stage = network_policy::Stage::Ask; break;
  case MWMNetworkPolicyPermissionAlways: stage = network_policy::Stage::Always; break;
  case MWMNetworkPolicyPermissionNever: stage = network_policy::Stage::Never; break;
  case MWMNetworkPolicyPermissionToday: stage = network_policy::Stage::Today; break;
  case MWMNetworkPolicyPermissionNotToday: stage = network_policy::Stage::NotToday; break;
  }
  network_policy::SetStage(stage);
}

- (NSDate *)permissionExpirationDate
{
  return network_policy::GetPolicyDate();
}

- (BOOL)canUseNetwork
{
  return network_policy::CanUseNetwork();
}

- (MWMConnectionType)connectionType
{
  switch (GetPlatform().ConnectionStatus())
  {
  case Platform::EConnectionType::CONNECTION_NONE: return MWMConnectionTypeNone;
  case Platform::EConnectionType::CONNECTION_WIFI: return MWMConnectionTypeWifi;
  case Platform::EConnectionType::CONNECTION_WWAN: return MWMConnectionTypeCellular;
  }
}

@end
