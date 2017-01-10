#import "MWMSearchFilterTransitioningManager.h"
#import "MWMSearchFilterPresentationController.h"
#import "MWMSearchFilterTransitioning.h"

@implementation MWMSearchFilterTransitioningManager

- (UIPresentationController *)
presentationControllerForPresentedViewController:(UIViewController *)presented
                        presentingViewController:(UIViewController *)presenting
                            sourceViewController:(UIViewController *)source
{
  return [[MWMSearchFilterPresentationController alloc] initWithPresentedViewController:presented
                                                               presentingViewController:presenting];
}

- (id<UIViewControllerAnimatedTransitioning>)
animationControllerForPresentedController:(UIViewController *)presented
                     presentingController:(UIViewController *)presenting
                         sourceController:(UIViewController *)source
{
  auto animationController = [[MWMSearchFilterTransitioning alloc] init];
  animationController.isPresentation = YES;
  return animationController;
}

- (id<UIViewControllerAnimatedTransitioning>)animationControllerForDismissedController:
    (UIViewController *)dismissed
{
  auto animationController = [[MWMSearchFilterTransitioning alloc] init];
  animationController.isPresentation = NO;
  return animationController;
}

@end
