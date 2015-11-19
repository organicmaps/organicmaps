#import "MWMPageController.h"
#import "MWMWhatsNewController.h"

static NSString * const kPageViewControllerStoryboardID = @"PageViewController";
static NSString * const kContentViewControllerStoryboardID = @"PageContentController";
static NSUInteger const kNumberOfPages = 2;

@protocol MWMPageControllerDataSource <UIPageViewControllerDataSource>

- (MWMWhatsNewController *)viewControllerAtIndex:(NSUInteger)index;

@end

NS_CLASS_AVAILABLE_IOS(8_0) @interface MWMPageControllerDataSourceImpl : NSObject <MWMPageControllerDataSource>

@property (weak, nonatomic, readonly) MWMPageController * pageController;

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

- (UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerBeforeViewController:(UIViewController *)viewController
{
  NSUInteger index = [(MWMWhatsNewController *)viewController pageIndex];

  if ((index == 0) || (index == NSNotFound))
    return nil;

  index--;
  return [self viewControllerAtIndex:index];
}

- (UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerAfterViewController:(UIViewController *)viewController
{
  NSUInteger index = [(MWMWhatsNewController *)viewController pageIndex];

  if (index == NSNotFound)
    return nil;

  index++;
  if (index == kNumberOfPages)
    return nil;

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
  [self.iPadBackgroundView removeFromSuperview];
  [self.view removeFromSuperview];
  [self removeFromParentViewController];
}

- (void)nextPage
{
  [self setViewControllers:@[[self.pageControllerDataSource viewControllerAtIndex:1]] direction:UIPageViewControllerNavigationDirectionForward animated:YES completion:nil];
}

- (void)show
{
  if (IPAD)
    [self.parent.view addSubview:self.iPadBackgroundView];
  [self.parent addChildViewController:self];
  [self.parent.view addSubview:self.view];
  [self didMoveToParentViewController:self.parent];
}

- (CGSize)defaultSize
{
  return IPAD ? CGSizeMake(520.0, 600.0) : self.parent.view.frame.size;
}

#pragma mark Private methods

- (void)configure
{
  UIView * mainView = self.view;
  UIView * parentView = self.parent.view;
  CGSize const size = self.defaultSize;
  CGPoint const origin = IPAD ? CGPointMake(parentView.center.x - size.width / 2, parentView.center.y - size.height / 2) : CGPointZero;
  mainView.frame = {origin, size};
  mainView.backgroundColor = [UIColor whiteColor];
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
    self.view.center = {size.width / 2, size.height / 2 };
  else
    self.view.origin = {};
}

@end
