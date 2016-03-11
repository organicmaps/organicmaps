#import "MWMPageController.h"
#import "MWMPageControllerDataSource.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"

@interface MWMPageController ()

@property (nonatomic) SolidTouchView * iPadBackgroundView;
@property (weak, nonatomic) UIViewController * parent;

@property (nonatomic) MWMPageControllerDataSource * pageControllerDataSource;

@end

@implementation MWMPageController

+ (instancetype)pageControllerWithParent:(UIViewController *)parentViewController
{
  NSAssert(parentViewController != nil, @"Parent view controller can't be nil!");
  MWMPageController * pageController = [[MWMWelcomeController welcomeStoryboard]
      instantiateViewControllerWithIdentifier:[self className]];
  pageController.parent = parentViewController;
  return pageController;
}

- (void)close
{
  MWMWelcomeController * current = self.viewControllers.firstObject;
  [Statistics logEvent:kStatEventName(kStatWhatsNew, [[current class] udWelcomeWasShownKey])
        withParameters:@{kStatAction : kStatClose}];
  [self.iPadBackgroundView removeFromSuperview];
  [self.view removeFromSuperview];
  [self removeFromParentViewController];
}

- (void)nextPage
{
  MWMWelcomeController * current = self.viewControllers.firstObject;
  [Statistics logEvent:kStatEventName(kStatWhatsNew, [[current class] udWelcomeWasShownKey])
        withParameters:@{kStatAction : kStatNext}];
  MWMWelcomeController * next = static_cast<MWMWelcomeController *>(
      [self.pageControllerDataSource pageViewController:self
                      viewControllerAfterViewController:current]);
  [self setViewControllers:@[ next ]
                 direction:UIPageViewControllerNavigationDirectionForward
                  animated:YES
                completion:nil];
}

- (void)show:(Class<MWMWelcomeControllerProtocol>)welcomeClass
{
  [Statistics logEvent:kStatEventName(kStatWhatsNew, [welcomeClass udWelcomeWasShownKey])
      withParameters:@{kStatAction : kStatOpen}];
  [self configure:welcomeClass];
  if (IPAD)
    [self.parent.view addSubview:self.iPadBackgroundView];
  [self.parent addChildViewController:self];
  [self.parent.view addSubview:self.view];
  [self didMoveToParentViewController:self.parent];
}

#pragma mark - Private methods

- (void)configure:(Class<MWMWelcomeControllerProtocol>)welcomeClass
{
  UIView * mainView = self.view;
  UIView * parentView = self.parent.view;
  CGSize const size = IPAD ? CGSizeMake(520.0, 600.0) : parentView.size;
  CGPoint const origin = IPAD ? CGPointMake(parentView.center.x - size.width / 2,
                                            parentView.center.y - size.height / 2)
                              : CGPointZero;
  mainView.frame = {origin, size};
  mainView.backgroundColor = [UIColor white];
  if (IPAD)
  {
    self.iPadBackgroundView = [[SolidTouchView alloc] initWithFrame:parentView.bounds];
    self.iPadBackgroundView.backgroundColor = [UIColor colorWithWhite:0. alpha:0.67];
    self.iPadBackgroundView.autoresizingMask =
        UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
    mainView.layer.cornerRadius = 5.;
  }
  self.pageControllerDataSource =
      [[MWMPageControllerDataSource alloc] initWithPageController:self welcomeClass:welcomeClass];
  self.dataSource = self.pageControllerDataSource;
  [self setViewControllers:@[ [self.pageControllerDataSource firstWelcomeController] ]
                 direction:UIPageViewControllerNavigationDirectionReverse
                  animated:NO
                completion:nil];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
  {
    if (IPAD)
      self.view.center = {size.width / 2, size.height / 2};
    else
      self.view.origin = {};
  } completion:nil];
}

@end
