#import "MWMLocationAlert.h"
#import "MWMAlertViewController.h"

static NSString * const kLocationAlertNibName = @"MWMLocationAlert";

@interface MWMLocationAlert ()

@property (weak, nonatomic) IBOutlet UIButton * rightButton;
@property (nullable, nonatomic) MWMVoidBlock cancelBlock;

@end

@implementation MWMLocationAlert

+ (instancetype)alertWithCancelBlock:(MWMVoidBlock)cancelBlock
{
  MWMLocationAlert * alert =
      [NSBundle.mainBundle loadNibNamed:kLocationAlertNibName owner:nil options:nil].firstObject;
  [alert setNeedsCloseAlertAfterEnterBackground];
  alert.cancelBlock = cancelBlock;
  return alert;
}

- (IBAction)settingsTap
{
  [self close:^{
    NSURL * url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
    UIApplication * a = UIApplication.sharedApplication;
    if ([a canOpenURL:url])
      [a openURL:url options:@{} completionHandler:nil];
  }];
}

- (IBAction)closeTap
{
  [self close:self.cancelBlock];
}

@end
