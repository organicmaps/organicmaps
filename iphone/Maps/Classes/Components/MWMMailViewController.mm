#import "MWMMailViewController.h"
#import "UIColor+MapsMeColor.h"

@implementation MWMMailViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.navigationBar.tintColor = [UIColor whiteColor];
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  return [UIColor isNightMode] ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;
}

- (UIViewController *)childViewControllerForStatusBarStyle { return nil; }
@end
