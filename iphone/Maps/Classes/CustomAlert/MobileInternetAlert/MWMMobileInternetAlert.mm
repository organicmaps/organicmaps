#import "MWMMobileInternetAlert.h"
#import "MWMNetworkPolicy.h"
#import "Statistics.h"

using namespace network_policy;
using np = platform::NetworkPolicy;

namespace
{
NSString * const kStatisticsEvent = @"Mobile Internet Settings Alert";
}

@interface MWMMobileInternetAlert ()

@property(copy, nonatomic) MWMVoidBlock completionBlock;

@end

@implementation MWMMobileInternetAlert

+ (nonnull instancetype)alertWithBlock:(nonnull MWMVoidBlock)block
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMMobileInternetAlert * alert =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  alert.completionBlock = block;
  return alert;
}

- (IBAction)alwaysTap
{
  [Statistics logEvent:kStatMobileInternet withParameters:@{kStatValue : kStatAlways}];
  SetStage(Stage::Always);
  [self close:self.completionBlock];
}

- (IBAction)askTap
{
  [Statistics logEvent:kStatMobileInternet withParameters:@{kStatValue: kStatToday}];
  SetStage(Stage::Today);
  [self close:self.completionBlock];
}

- (IBAction)neverTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction: kStatNotToday}];
  SetStage(Stage::NotToday);
  [self close:self.completionBlock];
}

@end
