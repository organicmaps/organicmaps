#import "MWMMailViewController.h"
#import "MWMCommon.h"

@implementation MWMMailViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.navigationBar.tintColor = UIColor.whiteColor;
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  setStatusBarBackgroundColor(UIColor.clearColor);
  return [UIColor isNightMode] ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;
}

- (UIViewController *)childViewControllerForStatusBarStyle { return nil; }
@end
