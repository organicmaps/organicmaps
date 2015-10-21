#import "Common.h"
#import "MWMBasePlacePageView.h"
#import "MWMBookmarkColorViewController.h"
#import "MWMBookmarkDescriptionViewController.h"
#import "MWMiPadPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageViewManager.h"
#import "SelectSetVC.h"
#import "UIViewController+Navigation.h"
#import "ViewController.h"

static CGFloat const kLeftOffset = 12.;
static CGFloat const kTopOffset = 36.;
static CGFloat const kBottomOffset = 60.;
static CGFloat const kKeyboardOffset = 12.;

@interface MWMiPadPlacePageViewController : ViewController

@end

@implementation MWMiPadPlacePageViewController

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self.navigationController setNavigationBarHidden:YES];
  self.view.autoresizingMask = UIViewAutoresizingNone;
}

@end

@interface MWMiPadNavigationController : UINavigationController

@end

@implementation MWMiPadNavigationController

- (instancetype)initWithViews:(NSArray *)views
{
  MWMiPadPlacePageViewController * viewController = [[MWMiPadPlacePageViewController alloc] init];
  if (!viewController)
    return nil;
  [views enumerateObjectsUsingBlock:^(UIView * view, NSUInteger idx, BOOL *stop)
  {
    [viewController.view addSubview:view];
  }];
  self = [super initWithRootViewController:viewController];
  if (!self)
    return nil;
  [self setNavigationBarHidden:YES];
  [self.navigationBar setTranslucent:NO];
  self.view.autoresizingMask = UIViewAutoresizingNone;
  return self;
}

- (void)backTap:(id)sender
{
  [self popViewControllerAnimated:YES];
}

- (void)pushViewController:(UIViewController *)viewController animated:(BOOL)animated
{
  viewController.view.frame = self.view.bounds;
  [super pushViewController:viewController animated:animated];
}

- (UIViewController *)popViewControllerAnimated:(BOOL)animated
{
  NSUInteger const count = self.viewControllers.count;
  CGFloat const height = count > 1 ? ((UIViewController *)self.viewControllers[count - 2]).view.height : 0.0;

  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.view.height = height;
  }
  completion:^(BOOL finished)
  {
    [super popViewControllerAnimated:animated];
  }];
  return self.viewControllers.lastObject;
}

@end

@interface MWMiPadPlacePage ()

@property (nonatomic) MWMiPadNavigationController * navigationController;
@property (nonatomic) CGFloat height;

@end

@implementation MWMiPadPlacePage

- (void)configure
{
  [super configure];

  CGFloat const defaultWidth = 360.;
  [self updatePlacePageLayoutAnimated:NO];
  [self addPlacePageShadowToView:self.navigationController.view offset:CGSizeMake(0.0, -2.0)];

  [self.manager addSubviews:@[self.navigationController.view] withNavigationController:self.navigationController];
  self.navigationController.view.frame = CGRectMake( -defaultWidth, self.topBound + kTopOffset, defaultWidth, self.height);
  self.extendedPlacePageView.frame = CGRectMake(0., 0., defaultWidth, self.height);
  self.anchorImageView.image = nil;
  self.anchorImageView.backgroundColor = [UIColor whiteColor];
  self.actionBar.width = defaultWidth;
  [self configureContentInset];
}

- (void)show
{
  UIView * view = self.navigationController.view;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    view.minY = self.topBound + kTopOffset;
    view.minX = self.leftBound + kLeftOffset;
    view.alpha = 1.0;
  }];
}

- (void)hide
{
  [self.manager dismissPlacePage];
}

- (void)dismiss
{
  UIView * view = self.navigationController.view;
  UIViewController * controller = self.navigationController;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    view.maxX = 0.0;
    view.alpha = 0.0;
  }
  completion:^(BOOL finished)
  {
    [view removeFromSuperview];
    [controller removeFromParentViewController];
    [super dismiss];
  }];
}

- (void)willStartEditingBookmarkTitle
{
  [self updatePlacePagePosition];
}

- (void)willFinishEditingBookmarkTitle:(NSString *)title
{
  [super willFinishEditingBookmarkTitle:title];
  [self updatePlacePageLayoutAnimated:NO];
}

- (void)addBookmark
{
  [super addBookmark];
  [self updatePlacePageLayoutAnimated:YES];
}

- (void)removeBookmark
{
  [super removeBookmark];
  [self updatePlacePageLayoutAnimated:YES];
}

- (void)reloadBookmark
{
  [super reloadBookmark];
  [self updatePlacePageLayoutAnimated:YES];
}

- (void)changeBookmarkColor
{
  MWMBookmarkColorViewController * controller = [[MWMBookmarkColorViewController alloc] initWithNibName:[MWMBookmarkColorViewController className] bundle:nil];
  controller.iPadOwnerNavigationController = self.navigationController;
  controller.placePageManager = self.manager;
  [self.navigationController pushViewController:controller animated:YES];
}

- (void)changeBookmarkCategory
{
  SelectSetVC * controller = [[SelectSetVC alloc] initWithPlacePageManager:self.manager];
  controller.iPadOwnerNavigationController = self.navigationController;
  [self.navigationController pushViewController:controller animated:YES];
}

- (void)changeBookmarkDescription
{
  MWMBookmarkDescriptionViewController * controller = [[MWMBookmarkDescriptionViewController alloc] initWithPlacePageManager:self.manager];
  controller.iPadOwnerNavigationController = self.navigationController;
  [self.navigationController pushViewController:controller animated:YES];
  [self updatePlacePageLayoutAnimated:NO];
}

- (IBAction)didPan:(UIPanGestureRecognizer *)sender
{
  UIView * view = self.navigationController.view;
  UIView * superview = view.superview;

  CGFloat const leftOffset = self.leftBound + kLeftOffset;
  view.minX += [sender translationInView:superview].x;
  view.minX = MIN(view.minX, leftOffset);
  [sender setTranslation:CGPointZero inView:superview];

  CGFloat const alpha = MAX(0.0, view.maxX) / (view.width + leftOffset);
  view.alpha = alpha;
  UIGestureRecognizerState const state = sender.state;
  if (state == UIGestureRecognizerStateEnded || state == UIGestureRecognizerStateCancelled)
  {
    if (alpha < 0.8)
      [self.manager dismissPlacePage];
    else
      [self show];
  }
}

- (void)updatePlacePageLayoutAnimated:(BOOL)animated
{
  [UIView animateWithDuration:animated ? kDefaultAnimationDuration : 0.0 animations:^
  {
    CGFloat const actionBarHeight = self.actionBar.height;
    self.height = self.basePlacePageView.height + self.anchorImageView.height + actionBarHeight - 1;
    self.actionBar.origin = CGPointMake(0., self.height - actionBarHeight);
    [self updatePlacePagePosition];
  }];
}

- (void)updatePlacePagePosition
{
  UIView * view = self.navigationController.view;
  view.minY = MIN([self getAvailableHeight] + kTopOffset - view.height, self.topBound + kTopOffset);
  [self configureContentInset];
}

- (void)configureContentInset
{
  UITableView * featureTable = self.basePlacePageView.featureTable;
  CGFloat const height = self.navigationController.view.height;
  CGFloat const tableContentHeight = featureTable.contentSize.height;
  CGFloat const headerHeight = self.basePlacePageView.separatorView.maxY;
  CGFloat const actionBarHeight = self.actionBar.height;
  CGFloat const anchorHeight = self.anchorImageView.height;
  CGFloat const availableTableHeight = height - headerHeight - actionBarHeight - anchorHeight;
  CGFloat const externalHeight = tableContentHeight - availableTableHeight;
  if (externalHeight > 0.)
  {
    featureTable.contentInset = UIEdgeInsetsMake(0., 0., externalHeight, 0.);
    featureTable.scrollEnabled = YES;
  }
  else
  {
    featureTable.contentInset = UIEdgeInsetsZero;
    featureTable.scrollEnabled = NO;
  }
}

- (void)keyboardWillChangeFrame:(NSNotification *)aNotification
{
  [super keyboardWillChangeFrame:aNotification];
  [self updateHeight];
}

- (CGFloat)getAvailableHeight
{
  CGFloat const bottomOffset = self.keyboardHeight > 0.0 ? kKeyboardOffset : kBottomOffset;
  CGFloat const availableHeight = self.parentViewHeight - self.keyboardHeight - kTopOffset - bottomOffset;
  return availableHeight;
}

#pragma mark - Properties

- (void)setHeight:(CGFloat)height
{
  _height = height;
  [self updateHeight];
}

- (void)updateHeight
{
  _height = MIN(_height, [self getAvailableHeight]);
  self.navigationController.view.height = _height;
  self.extendedPlacePageView.height = _height;
}

- (MWMiPadNavigationController *)navigationController
{
  if (!_navigationController)
  {
    _navigationController = [[MWMiPadNavigationController alloc] initWithViews:@[self.extendedPlacePageView, self.actionBar]];
  }
  return _navigationController;
}

- (void)setTopBound:(CGFloat)topBound
{
  super.topBound = topBound;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.navigationController.view.minY = topBound + kTopOffset;
  }];
}

- (void)setLeftBound:(CGFloat)leftBound
{
  super.leftBound = leftBound;
  [UIView animateWithDuration:kDefaultAnimationDuration animations:^
  {
    self.navigationController.view.minX = leftBound + kLeftOffset;
  }];
}
@end
