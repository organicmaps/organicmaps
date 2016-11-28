#import "MWMNetworkingPolicy.h"
#import "MWMAlertViewController.h"

namespace
{
NSString * const kNetworkingPolicyTimeStamp = @"NetworkingPolicyTimeStamp";
NSTimeInterval const kSessionDurationSeconds = 24 * 60 * 60;
}  // namespace

namespace networking_policy
{
void callPartnersApiWithNetworkingPolicy(MWMNetworkingPolicyFn const & fn)
{
  auto checkAndApply = ^BOOL {
    NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
    NSDate * policyDate = [ud objectForKey:kNetworkingPolicyTimeStamp];
    if ([policyDate compare:[NSDate date]] == NSOrderedDescending)
    {
      fn(YES);
      return YES;
    }
    if ([policyDate isEqualToDate:NSDate.distantPast])
    {
      fn(NO);
      return YES;
    }
    return NO;
  };

  if (checkAndApply())
    return;

  MWMAlertViewController * alertController = [MWMAlertViewController activeAlertController];
  [alertController presentMobileInternetAlertWithBlock:^{
    if (!checkAndApply())
      fn(NO);
  }];
}

void SetNetworkingPolicyState(NetworkingPolicyState state)
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  NSDate * policyDate = nil;
  switch (state)
  {
  case NetworkingPolicyState::Always: policyDate = NSDate.distantFuture; break;
  case NetworkingPolicyState::Session:
    policyDate = [NSDate dateWithTimeIntervalSinceNow:kSessionDurationSeconds];
    break;
  case NetworkingPolicyState::Never: policyDate = NSDate.distantPast; break;
  }
  [ud setObject:policyDate forKey:kNetworkingPolicyTimeStamp];
}

NetworkingPolicyState GetNetworkingPolicyState()
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  NSDate * policyDate = [ud objectForKey:kNetworkingPolicyTimeStamp];
  if ([policyDate isEqualToDate:NSDate.distantFuture])
    return NetworkingPolicyState::Always;
  if ([policyDate isEqualToDate:NSDate.distantPast])
    return NetworkingPolicyState::Never;
  return NetworkingPolicyState::Session;
}
}  // namespace networking_policy
