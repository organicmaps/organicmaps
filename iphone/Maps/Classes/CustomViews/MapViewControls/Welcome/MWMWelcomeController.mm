#import "MWMWelcomeController.h"

@implementation MWMWelcomeController

+ (UIStoryboard *)welcomeStoryboard
{
  return [UIStoryboard storyboardWithName:@"Welcome" bundle:[NSBundle mainBundle]];
}

+ (instancetype)welcomeController
{
  return [[self welcomeStoryboard] instantiateViewControllerWithIdentifier:[self className]];
}

+ (NSInteger)pagesCount
{
  return [self pagesConfig].count;
}

+ (NSString *)udWelcomeWasShownKey
{
  [self doesNotRecognizeSelector:_cmd];
  return nil;
}

+ (NSArray<TMWMWelcomeConfigBlock> *)pagesConfig
{
  [self doesNotRecognizeSelector:_cmd];
  return nil;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.size = self.parentViewController.view.size;
  [self configPage];
}

- (void)configPage
{
  [[self class] pagesConfig][self.pageIndex](self);
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
  {
    self.size = size;
  } completion:nil];
}

#pragma mark - Properties

- (void)setSize:(CGSize)size
{
  CGSize const iPadSize = {520.0, 534.0};
  CGSize const newSize = IPAD ? iPadSize : size;
  self.parentViewController.view.size = newSize;
}

- (CGSize)size
{
  return self.parentViewController.view.size;
}

@end
