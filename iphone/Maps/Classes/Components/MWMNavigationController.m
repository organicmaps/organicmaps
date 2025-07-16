#import "MWMNavigationController.h"
#import "MWMController.h"
#import "SwiftBridge.h"

#import <SafariServices/SafariServices.h>

@interface MWMNavigationController () <UINavigationControllerDelegate>

@end

@implementation MWMNavigationController

- (UIStatusBarStyle)preferredStatusBarStyle
{
  return UIStatusBarStyleLightContent;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.delegate = self;
  self.navigationItem.leftBarButtonItem.tintColor = [UIColor whitePrimaryText];
  self.navigationItem.rightBarButtonItem.tintColor = [UIColor whitePrimaryText];

  [MWMThemeManager invalidate];
}

- (void)navigationController:(UINavigationController *)navigationController
      willShowViewController:(UIViewController *)viewController
                    animated:(BOOL)animated
{
  if ([viewController isKindOfClass:[SFSafariViewController class]])
  {
    [navigationController setNavigationBarHidden:YES animated:animated];
    return;
  }

  NSAssert([viewController conformsToProtocol:@protocol(MWMController)],
           @"Controller must inherit ViewController or TableViewController class");
  id<MWMController> vc = (id<MWMController>)viewController;
  [navigationController setNavigationBarHidden:!vc.hasNavigationBar animated:animated];
}

- (void)pushViewController:(UIViewController *)viewController animated:(BOOL)animated
{
  UIViewController * topVC = self.viewControllers.lastObject;
  [self setupNavigationBackButtonItemFor:topVC];
  [super pushViewController:viewController animated:animated];
}

- (void)setViewControllers:(NSArray<UIViewController *> *)viewControllers animated:(BOOL)animated
{
  [viewControllers enumerateObjectsUsingBlock:^(UIViewController * vc, NSUInteger idx, BOOL * stop) {
    if (idx == viewControllers.count - 1)
      return;
    [self setupNavigationBackButtonItemFor:vc];
  }];
  [super setViewControllers:viewControllers animated:animated];
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection
{
  [super traitCollectionDidChange:previousTraitCollection];
  // Update the app theme when the device appearance is changing.
  if ((self.traitCollection.verticalSizeClass != previousTraitCollection.verticalSizeClass) ||
      (self.traitCollection.horizontalSizeClass != previousTraitCollection.horizontalSizeClass) ||
      (self.traitCollection.userInterfaceStyle != previousTraitCollection.userInterfaceStyle))
  {
    [MWMThemeManager invalidate];
  }
}

- (BOOL)shouldAutorotate
{
  return YES;
}

- (void)setupNavigationBackButtonItemFor:(UIViewController *)viewController
{
  if (@available(iOS 14.0, *))
  {
    viewController.navigationItem.backButtonDisplayMode = UINavigationItemBackButtonDisplayModeMinimal;
  }
  else
  {
    viewController.navigationItem.backBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:@""
                                                                                       style:UIBarButtonItemStylePlain
                                                                                      target:nil
                                                                                      action:nil];
  }
}

@end
