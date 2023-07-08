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

  if (@available(iOS 13.0, *)) {
    [MWMThemeManager setDarkModeEnabled: self.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark];
  }
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

  NSAssert([viewController conformsToProtocol:@protocol(MWMController)], @"Controller must inherit ViewController or TableViewController class");
  id<MWMController> vc = (id<MWMController>)viewController;
  [navigationController setNavigationBarHidden:!vc.hasNavigationBar animated:animated];
}

- (void)pushViewController:(UIViewController *)viewController animated:(BOOL)animated
{
  UIViewController * topVC = self.viewControllers.lastObject;
  topVC.navigationItem.backBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:@""
                                                                            style:UIBarButtonItemStylePlain
                                                                           target:nil
                                                                           action:nil];
  [super pushViewController:viewController animated:animated];
}

- (void)setViewControllers:(NSArray<UIViewController *> *)viewControllers animated:(BOOL)animated {
  [viewControllers enumerateObjectsUsingBlock:^(UIViewController * vc, NSUInteger idx, BOOL * stop) {
    if (idx == viewControllers.count - 1)
      return;

    vc.navigationItem.backBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:@""
                                                                           style:UIBarButtonItemStylePlain
                                                                          target:nil
                                                                          action:nil];
  }];
  [super setViewControllers:viewControllers animated:animated];
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection
{
  [super traitCollectionDidChange: previousTraitCollection];
  if (@available(iOS 13.0, *)) {
    if (self.traitCollection.userInterfaceStyle != previousTraitCollection.userInterfaceStyle) {
      [MWMThemeManager setDarkModeEnabled: self.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark];
    }
  }
}

- (BOOL)shouldAutorotate
{
  return YES;
}

@end
