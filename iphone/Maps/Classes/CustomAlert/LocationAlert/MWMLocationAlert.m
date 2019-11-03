#import "MWMLocationAlert.h"
#import "MWMAlertViewController.h"
#import "Statistics.h"

static NSString * const kLocationAlertNibName = @"MWMLocationAlert";
static NSString * const kStatisticsEvent = @"Location Alert";

@interface MWMLocationAlert ()

@property (weak, nonatomic) IBOutlet UIButton * rightButton;
@property (nullable, nonatomic) MWMVoidBlock cancelBlock;

@end

@implementation MWMLocationAlert

+ (instancetype)alertWithCancelBlock:(MWMVoidBlock)cancelBlock
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMLocationAlert * alert =
      [NSBundle.mainBundle loadNibNamed:kLocationAlertNibName owner:nil options:nil].firstObject;
  [alert setNeedsCloseAlertAfterEnterBackground];
  alert.cancelBlock = cancelBlock;
  return alert;
}

- (IBAction)settingsTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatApply}];
  [self close:^{
    NSURL * url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
    UIApplication * a = UIApplication.sharedApplication;
    if ([a canOpenURL:url])
      [a openURL:url options:@{} completionHandler:nil];
  }];
}

- (IBAction)closeTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatClose}];
  [self close:self.cancelBlock];
}

@end
