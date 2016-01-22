#import "Common.h"
#import "MWMBasePlacePageView.h"
#import "MWMBookmarkColorViewController.h"
#import "MWMBookmarkDescriptionViewController.h"
#import "MWMiPadPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageViewManager.h"
#import "SelectSetVC.h"
#import "UIColor+MapsMeColor.h"
#import "UIViewController+Navigation.h"
#import "ViewController.h"

static CGFloat const kLeftOffset = 12.;
static CGFloat const kTopOffset = 36.;
static CGFloat const kBottomOffset = 60.;
static CGFloat const kKeyboardOffset = 12.;

@interface MWMiPadPlacePageViewController : ViewController

@property (nonatomic) UIView * placePageView;
@property (nonatomic) UIView * actionBarView;

@end

@implementation MWMiPadPlacePageViewController

- (instancetype)initWithPlacepageView:(UIView *)ppView actionBar:(UIView *)actionBar
{
  self = [super init];
  if (self)
  {
    self.view.backgroundColor = [UIColor white];
    self.placePageView = ppView;
    self.actionBarView = actionBar;
    [self.view addSubview:ppView];
    [self.view addSubview:actionBar];
  }
  return self;
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self.navigationController setNavigationBarHidden:YES];
  self.view.autoresizingMask = UIViewAutoresizingNone;
}

@end

@interface MWMiPadNavigationController : UINavigationController

@property (weak, nonatomic) MWMiPadPlacePage * placePage;
@property (nonatomic) CGFloat height;

@end

@implementation MWMiPadNavigationController

- (instancetype)initWithViewController:(UIViewController *)viewController frame:(CGRect)frame
{
  self = [super initWithRootViewController:viewController];
  if (self)
  {
    self.view.frame = viewController.view.frame = frame;
    [self setNavigationBarHidden:YES];
    [self.navigationBar setTranslucent:NO];
    self.view.autoresizingMask = UIViewAutoresizingNone;
  }
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
  [self.view endEditing:YES];
  NSUInteger const count = self.viewControllers.count;
  UIViewController * viewController = self.viewControllers.lastObject;
  if (count == 2)
  {
    [super popViewControllerAnimated:animated];
    [self.placePage updatePlacePageLayoutAnimated:YES];
  }
  else
  {
    CGFloat const height = count > 1 ? ((UIViewController *)self.viewControllers[count - 2]).view.height : 0.0;

    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      self.height = height;
    }
    completion:^(BOOL finished)
    {
      [super popViewControllerAnimated:animated];
    }];
  }
  return viewController;
}

- (void)setHeight:(CGFloat)height
{
  self.view.height = height;
  UIViewController * vc = self.topViewController;
  vc.view.height = height;
  if ([vc isKindOfClass:[MWMiPadPlacePageViewController class]])
  {
    MWMiPadPlacePageViewController * ppVC = (MWMiPadPlacePageViewController *)vc;
    ppVC.placePageView.height = height;
    ppVC.actionBarView.origin = {0, height - ppVC.actionBarView.height};
  }
}

- (CGFloat)height
{
  return self.view.height;
}

@end

@interface MWMiPadPlacePage ()

@property (nonatomic) MWMiPadNavigationController * navigationController;

@end

@implementation MWMiPadPlacePage

- (void)configure
{
  [super configure];

  CGFloat const defaultWidth = 360.;
  CGFloat const actionBarHeight = self.actionBar.height;
  self.height =
      self.basePlacePageView.height + self.anchorImageView.height + actionBarHeight - 1;
  self.extendedPlacePageView.frame = {{0, 0}, {defaultWidth, self.height}};
  self.actionBar.frame = {{0, self.height - actionBarHeight},{defaultWidth, actionBarHeight}};
  MWMiPadPlacePageViewController * viewController =
      [[MWMiPadPlacePageViewController alloc] initWithPlacepageView:self.extendedPlacePageView
                                                          actionBar:self.actionBar];
  self.navigationController = [[MWMiPadNavigationController alloc]
      initWithViewController:viewController
                       frame:{{-defaultWidth, self.topBound + kTopOffset},
                              {defaultWidth, self.height}}];
  self.navigationController.placePage = self;
  [self updatePlacePagePosition];
  [self addPlacePageShadowToView:self.navigationController.view offset:{0, -2}];

  [self.manager addSubviews:@[ self.navigationController.view ]
      withNavigationController:self.navigationController];
  self.anchorImageView.image = nil;
  self.anchorImageView.backgroundColor = [UIColor white];
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
  [super willStartEditingBookmarkTitle];
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
  if (![self.navigationController.topViewController isKindOfClass:[MWMiPadPlacePageViewController class]])
    return;
  [UIView animateWithDuration:animated ? kDefaultAnimationDuration : 0.0 animations:^
  {
    CGFloat const ppHeight = self.basePlacePageView.height;
    CGFloat const anchorHeight = self.anchorImageView.height;
    CGFloat const actionBarHeight = self.actionBar.height;
    self.height = ppHeight + anchorHeight + actionBarHeight - 1;
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

- (void)keyboardWillShow:(NSNotification *)aNotification
{
  [super keyboardWillShow:aNotification];
  [self updatePlacePageLayoutAnimated:YES];
}

- (void)keyboardWillHide
{
  [super keyboardWillHide];
  [self updatePlacePageLayoutAnimated:YES];
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
  _height = self.navigationController.height = MIN(height, [self getAvailableHeight]);
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
