#import "MWMPageController.h"
#import "MWMPageControllerDataSource.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"

@interface MWMPageController ()

@property (nonatomic) SolidTouchView * iPadBackgroundView;
@property (weak, nonatomic) UIViewController<MWMPageControllerProtocol> * parent;

@property (nonatomic) MWMPageControllerDataSource * pageControllerDataSource;
@property (nonatomic) MWMWelcomeController * currentController;
@property (nonatomic) BOOL isAnimatingTransition;

@end

@implementation MWMPageController

+ (instancetype)pageControllerWithParent:(UIViewController<MWMPageControllerProtocol> *)parentViewController
{
  NSAssert(parentViewController != nil, @"Parent view controller can't be nil!");
  MWMPageController * pageController = [[MWMWelcomeController welcomeStoryboard]
      instantiateViewControllerWithIdentifier:[self className]];
  pageController.parent = parentViewController;
  return pageController;
}

- (void)close
{
  MWMWelcomeController * current = self.currentController;
  [Statistics logEvent:kStatEventName(kStatWhatsNew, [[current class] udWelcomeWasShownKey])
        withParameters:@{kStatAction : kStatClose}];
  [self.iPadBackgroundView removeFromSuperview];
  [self.view removeFromSuperview];
  [self removeFromParentViewController];
  [self.parent closePageController:self];
}

- (void)nextPage
{
  MWMWelcomeController * current = self.currentController;
  [Statistics logEvent:kStatEventName(kStatWhatsNew, [[current class] udWelcomeWasShownKey])
        withParameters:@{kStatAction : kStatNext}];
  self.currentController = static_cast<MWMWelcomeController *>(
      [self.pageControllerDataSource pageViewController:self
                      viewControllerAfterViewController:current]);
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
  self.currentController = [self.pageControllerDataSource firstWelcomeController];
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

#pragma mark - Properties

- (MWMWelcomeController *)currentController
{
  return self.viewControllers.firstObject;
}

- (void)setCurrentController:(MWMWelcomeController *)currentController
{
  if (!currentController || self.isAnimatingTransition)
    return;
  self.isAnimatingTransition = YES;
  __weak auto weakSelf = self;
  NSArray<UIViewController *> * viewControllers = @[ currentController ];
  UIPageViewControllerNavigationDirection const direction = UIPageViewControllerNavigationDirectionForward;
  [self setViewControllers:viewControllers
                 direction:direction
                  animated:YES
                completion:^(BOOL finished)
  {
    if (finished)
    {
      dispatch_async(dispatch_get_main_queue(), ^
      {
        weakSelf.isAnimatingTransition = NO;
        [weakSelf setViewControllers:viewControllers direction:direction animated:NO completion:nil];
      });
    }
  }];
}

@end
