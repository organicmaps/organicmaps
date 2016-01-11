#import "MWMPageController.h"
#import "MWMWhatsNewController.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"

#include "Framework.h"

namespace
{

NSString * const kPageViewControllerStoryboardID = @"PageViewController";
NSString * const kContentViewControllerStoryboardID = @"PageContentController";
NSUInteger const kNumberOfPages = 2;

} // namespace

extern NSString * const kUDWhatsNewWasShown;

@protocol MWMPageControllerDataSource <UIPageViewControllerDataSource>

- (MWMWhatsNewController *)viewControllerAtIndex:(NSUInteger)index;

@end

NS_CLASS_AVAILABLE_IOS(8_0) @interface MWMPageControllerDataSourceImpl : NSObject <MWMPageControllerDataSource>

@property (weak, nonatomic, readonly) MWMPageController * pageController;
@property (nonatomic) BOOL isButtonSelectedOnFirstScreen;

- (instancetype)initWithPageController:(MWMPageController *)pageController;

@end

@implementation MWMPageControllerDataSourceImpl

- (instancetype)initWithPageController:(MWMPageController *)pageController
{
  self = [super init];
  if (self)
    _pageController = pageController;
  return self;
}

- (NSInteger)presentationIndexForPageViewController:(UIPageViewController *)pageViewController
{
  return 0;
}

- (NSInteger)presentationCountForPageViewController:(UIPageViewController *)pageViewController
{
  return kNumberOfPages;
}

- (MWMWhatsNewController *)viewControllerAtIndex:(NSUInteger)index
{
  MWMWhatsNewController * vc = [self.storyboard instantiateViewControllerWithIdentifier:kContentViewControllerStoryboardID];
  vc.pageIndex = index;
  vc.pageController = self.pageController;
  return vc;
}

- (UIStoryboard *)storyboard
{
  return [UIStoryboard storyboardWithName:[NSBundle mainBundle].infoDictionary[@"UIMainStoryboardFile"]
                                   bundle:[NSBundle mainBundle]];
}

- (UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerBeforeViewController:(MWMWhatsNewController *)viewController
{
  NSUInteger index = viewController.pageIndex;
  viewController.enableButton.selected = self.isButtonSelectedOnFirstScreen;

  if (index == 0 || index == NSNotFound)
    return nil;

  index--;
  return [self viewControllerAtIndex:index];
}

- (UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerAfterViewController:(MWMWhatsNewController *)viewController
{
  NSUInteger index = viewController.pageIndex;
  viewController.enableButton.selected = NO;

  if (index == NSNotFound || index == kNumberOfPages - 1)
    return nil;

  index++;
  return [self viewControllerAtIndex:index];
}

@end

@interface MWMPageController ()

@property (nonatomic) SolidTouchView * iPadBackgroundView;
@property (weak, nonatomic) UIViewController * parent;
@property (nonatomic) MWMPageControllerDataSourceImpl * pageControllerDataSource;

@end

@implementation MWMPageController

+ (instancetype)pageControllerWithParent:(UIViewController *)parentViewController
{
  UIStoryboard * storyboard = [UIStoryboard storyboardWithName:[NSBundle mainBundle].infoDictionary[@"UIMainStoryboardFile"]
                                                        bundle:[NSBundle mainBundle]];
  NSAssert(parentViewController != nil, @"Parent view controller can't be nil!");
  MWMPageController * pageController = [storyboard instantiateViewControllerWithIdentifier:kPageViewControllerStoryboardID];
  pageController.parent = parentViewController;
  [pageController configure];
  return pageController;
}

- (void)close
{
  [[Statistics instance] logEvent:kStatEventName(kStatWhatsNew, kUDWhatsNewWasShown)
                   withParameters:@{kStatAction : kStatClose}];
  [self.iPadBackgroundView removeFromSuperview];
  [self.view removeFromSuperview];
  [self removeFromParentViewController];
}

- (void)nextPage
{
  [[Statistics instance] logEvent:kStatEventName(kStatWhatsNew, kUDWhatsNewWasShown)
                   withParameters:@{kStatAction : kStatNext}];
  [self setViewControllers:@[[self.pageControllerDataSource viewControllerAtIndex:1]] direction:UIPageViewControllerNavigationDirectionForward animated:YES completion:nil];
}

- (void)show
{
  [[Statistics instance] logEvent:kStatEventName(kStatWhatsNew, kUDWhatsNewWasShown)
                   withParameters:@{kStatAction : kStatOpen}];
  if (IPAD)
    [self.parent.view addSubview:self.iPadBackgroundView];
  [self.parent addChildViewController:self];
  [self.parent.view addSubview:self.view];
  [self didMoveToParentViewController:self.parent];
}

#pragma mark - Enabled methods

- (void)enableFirst:(UIButton *)button
{
  auto & f = GetFramework();
  bool _ = true, is3dBuildings = true;
  f.Load3dMode(_, is3dBuildings);
  BOOL const isEnabled = !button.selected;
  f.Save3dMode(_, isEnabled);
  f.Allow3dMode(_, isEnabled);
  button.selected = self.pageControllerDataSource.isButtonSelectedOnFirstScreen = isEnabled;
  [self nextPage];
}

- (void)enableSecond
{
  auto & f = GetFramework();
  bool _ = true;
  f.Load3dMode(_, _);
  f.Save3dMode(true, _);
  f.Allow3dMode(true, _);
  [self close];
}

- (void)skipFirst
{
  auto & f = GetFramework();
  bool _ = true, is3dBuildings = true;
  f.Load3dMode(_, is3dBuildings);
  f.Save3dMode(_, false);
  f.Allow3dMode(_, false);
  [self nextPage];
}

- (void)skipSecond
{
  auto & f = GetFramework();
  bool _ = true;
  f.Load3dMode(_, _);
  f.Save3dMode(false, _);
  f.Allow3dMode(false, _);
  [self close];
}

#pragma mark - Private methods

- (CGSize)defaultSize
{
  return IPAD ? CGSizeMake(520.0, 600.0) : self.parent.view.frame.size;
}

- (void)configure
{
  UIView * mainView = self.view;
  UIView * parentView = self.parent.view;
  CGSize const size = self.defaultSize;
  CGPoint const origin = IPAD ? CGPointMake(parentView.center.x - size.width / 2, parentView.center.y - size.height / 2) :
                                CGPointZero;
  mainView.frame = {origin, size};
  mainView.backgroundColor = [UIColor white];
  if (IPAD)
  {
    self.iPadBackgroundView = [[SolidTouchView alloc] initWithFrame:parentView.bounds];
    self.iPadBackgroundView.backgroundColor = [UIColor colorWithWhite:0. alpha:0.67];
    self.iPadBackgroundView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
    mainView.layer.cornerRadius = 5.;
  }
  self.pageControllerDataSource = [[MWMPageControllerDataSourceImpl alloc] init];
  self.dataSource = self.pageControllerDataSource;
  [self setViewControllers:@[[self.pageControllerDataSource viewControllerAtIndex:0]] direction:UIPageViewControllerNavigationDirectionReverse animated:NO completion:nil];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  if (IPAD)
    self.view.center = self.parent.view.center;
  else
    self.view.origin = {};
}

@end
