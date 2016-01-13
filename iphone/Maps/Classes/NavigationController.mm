
#import "NavigationController.h"
#import "MapsAppDelegate.h"
#import "UIViewController+Navigation.h"
#import "MapViewController.h"

@interface NavigationController () <UINavigationControllerDelegate>

@end

@implementation NavigationController

- (UIStatusBarStyle)preferredStatusBarStyle
{
  return UIStatusBarStyleLightContent;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.delegate = self;
}

- (void)navigationController:(UINavigationController *)navigationController willShowViewController:(UIViewController *)viewController animated:(BOOL)animated
{
  [navigationController setNavigationBarHidden:[viewController isMemberOfClass:[MapViewController class]] animated:animated];

  if ([navigationController.viewControllers count] > 1)
    [viewController showBackButton];
}

@end
