#import "MWMSearchFilterTransitioning.h"
#import "MWMCommon.h"

@implementation MWMSearchFilterTransitioning

- (instancetype)init
{
  self = [super init];
  if (self)
    _isPresentation = NO;
  return self;
}

- (NSTimeInterval)transitionDuration:(id<UIViewControllerContextTransitioning>)transitionContext
{
  return kDefaultAnimationDuration;
}

- (void)animateTransition:(id<UIViewControllerContextTransitioning>)transitionContext
{
  UIViewController * fromVC =
      [transitionContext viewControllerForKey:UITransitionContextFromViewControllerKey];
  UIViewController * toVC =
      [transitionContext viewControllerForKey:UITransitionContextToViewControllerKey];

  if (!toVC || !fromVC)
    return;

  UIView * fromView = fromVC.view;
  UIView * toView = toVC.view;
  UIView * containerView = [transitionContext containerView];

  UIViewController * animatingVC = self.isPresentation ? toVC : fromVC;
  UIView * animatingView = animatingVC.view;

  CGRect const finalFrameForVC = [transitionContext finalFrameForViewController:animatingVC];
  CGRect initialFrameForVC = finalFrameForVC;
  initialFrameForVC.origin.y += initialFrameForVC.size.height;

  CGRect const initialFrame = self.isPresentation ? initialFrameForVC : finalFrameForVC;
  CGRect const finalFrame = self.isPresentation ? finalFrameForVC : initialFrameForVC;

  animatingView.frame = initialFrame;

  if (self.isPresentation)
    [containerView addSubview:toView];

  [UIView animateWithDuration:[self transitionDuration:transitionContext]
      delay:0
      usingSpringWithDamping:300
      initialSpringVelocity:5.0
      options:UIViewAnimationOptionAllowUserInteraction
      animations:^{
        animatingView.frame = finalFrame;
      }
      completion:^(BOOL finished) {
        if (!self.isPresentation)
          [fromView removeFromSuperview];
        [transitionContext completeTransition:YES];
      }];
}

@end
