#import "MWMSearchFilterPresentationController.h"
#import "MWMCommon.h"
#import "MWMSearch.h"

namespace
{
CGFloat const kiPhonePortraitHeightPercentage = 0.7;

CGPoint originForParentSize(CGSize size)
{
  if (size.width > size.height)
    return {0, isIOSVersionLessThan(10) ? statusBarHeight() : 0};
  return {0, size.height * (1 - kiPhonePortraitHeightPercentage)};
}

CGSize sizeForParentSize(CGSize size)
{
  if (size.width > size.height)
    return {size.width, size.height - (isIOSVersionLessThan(10) ? statusBarHeight() : 0)};
  return {size.width, size.height * kiPhonePortraitHeightPercentage};
}
}  // namespace

@interface MWMSearchFilterPresentationController ()

@property(nonatomic) UIView * chromeView;

@end

@implementation MWMSearchFilterPresentationController

- (instancetype)initWithPresentedViewController:(UIViewController *)presentedViewController
                       presentingViewController:(UIViewController *)presentingViewController
{
  self = [super initWithPresentedViewController:presentedViewController
                       presentingViewController:presentingViewController];

  if (self)
  {
    _chromeView = [[UIView alloc] initWithFrame:{}];
    _chromeView.backgroundColor = [UIColor blackStatusBarBackground];
    _chromeView.alpha = 0;

    auto rec =
        [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(chromeViewTapped:)];
    [_chromeView addGestureRecognizer:rec];
  }

  return self;
}

#pragma mark - Gesture recognizers

- (void)chromeViewTapped:(UIGestureRecognizer *)gesture
{
  if (gesture.state != UIGestureRecognizerStateEnded)
    return;
  [MWMSearch update];
  [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
}

#pragma mark - Layout

- (CGRect)frameOfPresentedViewInContainerView
{
  auto const size = self.containerView.bounds.size;
  return {originForParentSize(size), sizeForParentSize(size)};
}

- (CGSize)sizeForChildContentContainer:(id<UIContentContainer>)container
               withParentContainerSize:(CGSize)parentSize
{
  return sizeForParentSize(parentSize);
}

- (void)containerViewWillLayoutSubviews
{
  self.chromeView.frame = self.containerView.bounds;
  self.presentedView.frame = [self frameOfPresentedViewInContainerView];
}

#pragma mark - Style

- (BOOL)shouldPresentInFullscreen { return YES; }
- (UIModalPresentationStyle)adaptivePresentationStyle { return UIModalPresentationFullScreen; }
#pragma mark - Presentation

- (void)presentationTransitionWillBegin
{
  UIView * chromeView = self.chromeView;
  UIView * containerView = self.containerView;

  chromeView.frame = containerView.bounds;
  chromeView.alpha = 0;

  [containerView insertSubview:chromeView atIndex:0];

  id<UIViewControllerTransitionCoordinator> coordinator =
      self.presentedViewController.transitionCoordinator;
  if (coordinator)
  {
    [coordinator
        animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
          chromeView.alpha = 1;
        }
                        completion:nil];
  }
  else
  {
    chromeView.alpha = 1;
  }
}

- (void)dismissalTransitionWillBegin
{
  UIView * chromeView = self.chromeView;
  id<UIViewControllerTransitionCoordinator> coordinator =
      self.presentedViewController.transitionCoordinator;
  if (coordinator)
  {
    [coordinator
        animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
          chromeView.alpha = 0;
        }
                        completion:nil];
  }
  else
  {
    chromeView.alpha = 0;
  }
}

@end
