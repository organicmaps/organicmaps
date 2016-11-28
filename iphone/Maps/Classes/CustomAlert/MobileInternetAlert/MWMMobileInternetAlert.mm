#import "MWMMobileInternetAlert.h"
#import "MWMNetworkingPolicy.h"
#import "Statistics.h"

using namespace networking_policy;

namespace
{
NSString * const kStatisticsEvent = @"Mobile Internet Settings Alert";
}

@interface MWMMobileInternetAlert ()

@property(copy, nonatomic) TMWMVoidBlock completionBlock;

@end

@implementation MWMMobileInternetAlert

+ (instancetype)alertWithBlock:(nonnull TMWMVoidBlock)block
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMMobileInternetAlert * alert =
      [[[NSBundle mainBundle] loadNibNamed:[MWMMobileInternetAlert className] owner:nil options:nil]
          firstObject];
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
  [Statistics logEvent:kStatMobileInternet withParameters:@{kStatValue : kStatAsk}];
  SetStage(Stage::Session);
  [self close:self.completionBlock];
}

- (IBAction)neverTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatNever}];
  SetStage(Stage::Never);
  [self close:self.completionBlock];
}

@end
