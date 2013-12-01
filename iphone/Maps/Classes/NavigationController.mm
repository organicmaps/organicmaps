
#import "NavigationController.h"
#import "MapsAppDelegate.h"
#import "UIViewController+Navigation.h"
#import "MapViewController.h"

@interface NavigationController () <UINavigationControllerDelegate>

@end

@implementation NavigationController

- (UIStatusBarStyle)preferredStatusBarStyle
{
  return [self.viewControllers count] > 1 ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;
}

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.delegate = self;
}

- (void)navigationController:(UINavigationController *)navigationController willShowViewController:(UIViewController *)viewController animated:(BOOL)animated
{
  if ([viewController isMemberOfClass:[MapViewController class]])
  {
    MapViewController * mapViewController = [MapsAppDelegate theApp].m_mapViewController;
    [navigationController setNavigationBarHidden:![mapViewController shouldShowNavBar] animated:YES];
  }
  else
  {
    [navigationController setNavigationBarHidden:NO animated:animated];
  }
  if ([navigationController.viewControllers count] > 1)
    [viewController showBackButton];
}

- (void)didReceiveMemoryWarning
{
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

@end
