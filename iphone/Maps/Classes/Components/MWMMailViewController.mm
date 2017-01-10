#import "MWMMailViewController.h"
#import "MWMCommon.h"
#import "MWMToast.h"
#import "UIColor+MapsMeColor.h"

@implementation MWMMailViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.navigationBar.tintColor = [UIColor whiteColor];
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  if ([MWMToast affectsStatusBar])
    return [MWMToast preferredStatusBarStyle];
  setStatusBarBackgroundColor([UIColor clearColor]);
  return [UIColor isNightMode] ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;
}

- (UIViewController *)childViewControllerForStatusBarStyle { return nil; }
@end
