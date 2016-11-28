#import "MWMMobileInternetAlert.h"
#import "Statistics.h"

namespace
{
NSString * const kStatisticsEvent = @"Mobile Internet Settings Alert";
}

@implementation MWMMobileInternetAlert

+ (instancetype)alert
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMMobileInternetAlert * alert =
      [[[NSBundle mainBundle] loadNibNamed:[MWMMobileInternetAlert className] owner:nil options:nil]
          firstObject];
  return alert;
}

- (IBAction)alwaysTap
{
  [Statistics logEvent:kStatMobileInternet withParameters:@{kStatValue : kStatAlways}];
  [self close:nil];
}

- (IBAction)askTap
{
  [Statistics logEvent:kStatMobileInternet withParameters:@{kStatValue : kStatAsk}];
  [self close:nil];
}

- (IBAction)neverTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatNever}];
  [self close:nil];
}

@end
