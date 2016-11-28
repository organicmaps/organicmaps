#import "MWMNetworkingPolicy.h"
#import "MWMAlertViewController.h"

namespace
{
NSString * const kNetworkingPolicyTimeStamp = @"NetworkingPolicyTimeStamp";
NSTimeInterval const kSessionDurationSeconds = 24 * 60 * 60;
}  // namespace

namespace networking_policy
{
void CallPartnersApi(MWMPartnersApiFn const & fn)
{
  auto checkAndApply = ^bool {
    NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
    NSDate * policyDate = [ud objectForKey:kNetworkingPolicyTimeStamp];
    if ([policyDate compare:[NSDate date]] == NSOrderedDescending)
    {
      fn(YES);
      return true;
    }
    if ([policyDate isEqualToDate:NSDate.distantPast])
    {
      fn(NO);
      return true;
    }
    return false;
  };

  if (checkAndApply())
    return;

  MWMAlertViewController * alertController = [MWMAlertViewController activeAlertController];
  [alertController presentMobileInternetAlertWithBlock:^{
    if (!checkAndApply())
      fn(NO);
  }];
}

void SetStage(Stage state)
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  NSDate * policyDate = nil;
  switch (state)
  {
  case Stage::Always: policyDate = NSDate.distantFuture; break;
  case Stage::Session:
    policyDate = [NSDate dateWithTimeIntervalSinceNow:kSessionDurationSeconds];
    break;
  case Stage::Never: policyDate = NSDate.distantPast; break;
  }
  [ud setObject:policyDate forKey:kNetworkingPolicyTimeStamp];
}

Stage GetStage()
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  NSDate * policyDate = [ud objectForKey:kNetworkingPolicyTimeStamp];
  if ([policyDate isEqualToDate:NSDate.distantFuture])
    return Stage::Always;
  if ([policyDate isEqualToDate:NSDate.distantPast])
    return Stage::Never;
  return Stage::Session;
}
}  // namespace networking_policy
