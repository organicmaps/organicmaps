#import "MWMCommon.h"
#import "MWMLocationAlert.h"
#import "MWMAlertViewController.h"
#import "Statistics.h"

static NSString * const kLocationAlertNibName = @"MWMLocationAlert";
static NSString * const kStatisticsEvent = @"Location Alert";

@interface MWMLocationAlert ()

@property (weak, nonatomic) IBOutlet UIButton * rightButton;

@end

@implementation MWMLocationAlert

+ (instancetype)alert
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMLocationAlert * alert =
      [NSBundle.mainBundle loadNibNamed:kLocationAlertNibName owner:nil options:nil].firstObject;
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

- (IBAction)settingsTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatApply}];
  [self close:^{
    NSURL * url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
    UIApplication * a = UIApplication.sharedApplication;
    if ([a canOpenURL:url])
      [a openURL:url];
  }];
}

- (IBAction)closeTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatClose}];
  [self close:nil];
}

@end
