#import "MWMEditorViralAlert.h"
#import "MWMActivityViewController.h"
#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "Statistics.h"
#import "SwiftBridge.h"

@interface MWMEditorViralAlert ()

@property(weak, nonatomic) IBOutlet UIButton* shareButton;

@end

@implementation MWMEditorViralAlert

+ (nonnull instancetype)alert {
  return [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
}

- (IBAction)shareTap {
  [Statistics logEvent:kStatEditorSecondTimeShareClick];
  [self close:^{
    MWMActivityViewController* shareVC = [MWMActivityViewController shareControllerForEditorViral];
    [shareVC presentInParentViewController:self.alertController.ownerViewController
                                anchorView:[BottomTabBarViewController controller].view];
  }];
}

- (IBAction)cancelTap {
  [self close:nil];
}

@end
