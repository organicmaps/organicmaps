#import "MWMPageController.h"
#import "MWMPageControllerDataSource.h"

@interface MWMPageControllerDataSource ()

@property (weak, nonatomic, readonly) MWMPageController * pageController;
@property (nonatomic) Class<MWMWelcomeControllerProtocol> welcomeClass;

@end

@implementation MWMPageControllerDataSource

- (instancetype)initWithPageController:(MWMPageController *)pageController welcomeClass:(Class<MWMWelcomeControllerProtocol>)welcomeClass
{
  self = [super init];
  if (self)
  {
    _pageController = pageController;
    _welcomeClass = welcomeClass;
  }
  return self;
}

- (MWMWelcomeController *)firstWelcomeController
{
  return [self welcomeControllerAtIndex:0];
}

- (MWMWelcomeController *)welcomeControllerAtIndex:(NSInteger)index
{
  if (index < 0 || index == NSNotFound || index >= [self.welcomeClass pagesCount])
    return nil;
  MWMWelcomeController * vc = [self.welcomeClass welcomeController];
  vc.pageIndex = index;
  vc.pageController = self.pageController;
  return vc;
}

#pragma mark - UIPageViewControllerDataSource

- (NSInteger)presentationIndexForPageViewController:(UIPageViewController *)pageViewController
{
  return static_cast<MWMWelcomeController *>(self.pageController.viewControllers.firstObject).pageIndex;
}

- (NSInteger)presentationCountForPageViewController:(UIPageViewController *)pageViewController
{
  return [self.welcomeClass pagesCount];
}

- (UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerBeforeViewController:(MWMWelcomeController *)viewController
{
  return [self welcomeControllerAtIndex:viewController.pageIndex - 1];
}

- (UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerAfterViewController:(MWMWelcomeController *)viewController
{
  return [self welcomeControllerAtIndex:viewController.pageIndex + 1];
}

@end
