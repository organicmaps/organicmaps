#import "MWMMailViewController.h"

@implementation MWMMailViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.navigationBar.tintColor = UIColor.whitePrimary;
  self.navigationBar.barTintColor = UIColor.greenPrimary;
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  return UIStatusBarStyleLightContent;
}

- (UIViewController *)childViewControllerForStatusBarStyle
{
  return nil;
}
@end
