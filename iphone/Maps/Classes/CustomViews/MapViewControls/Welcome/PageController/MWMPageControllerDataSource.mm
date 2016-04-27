#import "MWMPageController.h"
#import "MWMPageControllerDataSource.h"

@interface MWMPageControllerDataSource ()

@property (nonatomic) Class<MWMWelcomeControllerProtocol> welcomeClass;
@property (copy, nonatomic) NSArray<MWMWelcomeController *> * controllers;

@end

@implementation MWMPageControllerDataSource

- (instancetype)initWithWelcomeClass:(Class<MWMWelcomeControllerProtocol>)welcomeClass
{
  self = [super init];
  if (self)
  {
    _welcomeClass = welcomeClass;
    NSMutableArray<MWMWelcomeController *> * controllers = [@[] mutableCopy];
    for (NSInteger index = 0; index < [self.welcomeClass pagesCount]; ++index)
    {
      MWMWelcomeController * vc = [self.welcomeClass welcomeController];
      vc.pageIndex = index;
      [controllers addObject:vc];
    }
    self.controllers = controllers;
  }
  return self;
}

- (MWMWelcomeController *)firstWelcomeController
{
  return self.controllers.firstObject;
}

- (void)setPageController:(MWMPageController *)pageController
{
  for (MWMWelcomeController * vc in self.controllers)
    vc.pageController = pageController;
}

#pragma mark - UIPageViewControllerDataSource

- (NSInteger)presentationIndexForPageViewController:(UIPageViewController *)pageViewController
{
  return [self.controllers indexOfObject:pageViewController.viewControllers.firstObject];
}

- (NSInteger)presentationCountForPageViewController:(UIPageViewController *)pageViewController
{
  return [self.welcomeClass pagesCount];
}

- (UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerBeforeViewController:(MWMWelcomeController *)viewController
{
  NSUInteger const index = [self.controllers indexOfObject:viewController];
  NSAssert(index != NSNotFound, @"Invalid controller index");
  BOOL const isFirstController = (index == 0);
  return isFirstController ? nil : self.controllers[index - 1];
}

- (UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerAfterViewController:(MWMWelcomeController *)viewController
{
  NSUInteger const index = [self.controllers indexOfObject:viewController];
  NSAssert(index != NSNotFound, @"Invalid controller index");
  BOOL const isLastController = (index == [self.welcomeClass pagesCount] - 1);
  return isLastController ? nil : self.controllers[index + 1];
}

@end
