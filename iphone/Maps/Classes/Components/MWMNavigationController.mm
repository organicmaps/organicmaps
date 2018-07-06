#import "MWMNavigationController.h"
#import "MWMCommon.h"
#import "MWMController.h"
#import "MapsAppDelegate.h"
#import "UIViewController+Navigation.h"

#import <SafariServices/SafariServices.h>

@interface MWMNavigationController () <UINavigationControllerDelegate>

@end

@implementation MWMNavigationController

- (UIStatusBarStyle)preferredStatusBarStyle
{
  setStatusBarBackgroundColor(UIColor.clearColor);
  return UIStatusBarStyleLightContent;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.delegate = self;
  self.navigationItem.leftBarButtonItem.tintColor = [UIColor whitePrimaryText];
  self.navigationItem.rightBarButtonItem.tintColor = [UIColor whitePrimaryText];
}

- (void)navigationController:(UINavigationController *)navigationController willShowViewController:(UIViewController *)viewController animated:(BOOL)animated
{
  if ([viewController isKindOfClass:[SFSafariViewController class]])
  {
    [navigationController setNavigationBarHidden:YES animated:animated];
    return;
  }

  NSAssert([viewController conformsToProtocol:@protocol(MWMController)], @"Controller must inherit ViewController or TableViewController class");
  id<MWMController> vc = static_cast<id<MWMController>>(viewController);
  [navigationController setNavigationBarHidden:!vc.hasNavigationBar animated:animated];
}

- (void)pushViewController:(UIViewController *)viewController animated:(BOOL)animated
{
  auto topVC = self.viewControllers.lastObject;
  topVC.navigationItem.backBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:@""
                                                                            style:UIBarButtonItemStylePlain
                                                                           target:nil
                                                                           action:nil];
  [super pushViewController:viewController animated:animated];
}

- (BOOL)shouldAutorotate
{
  return YES;
}

@end
