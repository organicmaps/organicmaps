#import "MWMLocationAlert.h"
#import "MWMAlertViewController.h"
#import "Statistics.h"

static NSString * const kLocationAlertNibName = @"MWMLocationAlert";
static NSString * const kStatisticsEvent = @"Location Alert";

@implementation MWMLocationAlert

+ (instancetype)alert
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMLocationAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kLocationAlertNibName owner:nil options:nil] firstObject];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

- (IBAction)settingsTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatApply}];
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:UIApplicationOpenSettingsURLString]];
  [self close];
}

- (IBAction)closeTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatClose}];
  [self close];
}

@end
