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
  return UIStatusBarStyleLightContent;
}

- (UIViewController *)childViewControllerForStatusBarStyle { return nil; }
@end
