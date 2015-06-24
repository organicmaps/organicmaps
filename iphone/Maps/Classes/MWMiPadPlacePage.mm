//
//  MWMiPadPlacePageView.m
//  Maps
//
//  Created by v.mikhaylenko on 18.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMiPadPlacePage.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePageActionBar.h"
#import "UIKitCategories.h"
#import "MWMBasePlacePageView.h"
#import "MWMBookmarkColorViewController.h"
#import "SelectSetVC.h"
#import "UIViewController+Navigation.h"
#import "MWMBookmarkDescriptionViewController.h"

extern CGFloat kBookmarkCellHeight;
static CGFloat const kLeftOffset = 12.;
static CGFloat const kTopOffset = 36.;
static CGFloat const kBottomOffset = 12.;

@interface MWMiPadPlacePageViewController : UIViewController

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
  [self updatePlacePageLayout];
  [self addPlacePageShadowToView:self.navigationController.view];

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
  [UIView animateWithDuration:0.2f animations:^
  {
    view.minX = kLeftOffset;
    view.alpha = 1.0;
  }];
}

- (void)dismiss
{
  UIView * view = self.navigationController.view;
  UIViewController * controller = self.navigationController;
  [UIView animateWithDuration:0.2f animations:^
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

- (void)willStartEditingBookmarkTitle:(CGFloat)keyboardHeight
{
  [super willStartEditingBookmarkTitle:keyboardHeight];
  [self updatePlacePagePosition];
}

- (void)willFinishEditingBookmarkTitle:(NSString *)title
{
  [super willFinishEditingBookmarkTitle:title];
  [self updatePlacePageLayout];
}

- (void)addBookmark
{
  [super addBookmark];
  [UIView animateWithDuration:0.1 animations:^
  {
    [self updatePlacePageLayout];
  }];
}

- (void)removeBookmark
{
  [super removeBookmark];
  [UIView animateWithDuration:0.1 animations:^
  {
    [self updatePlacePageLayout];
  }];
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
}

- (IBAction)didPan:(UIPanGestureRecognizer *)sender
{
  UIView * view = self.navigationController.view;
  UIView * superview = view.superview;

  view.minX += [sender translationInView:superview].x;
  view.minX = MIN(view.minX, kLeftOffset);
  [sender setTranslation:CGPointZero inView:superview];

  CGFloat const alpha = MAX(0.0, view.maxX) / (view.width + kLeftOffset);
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

- (void)updatePlacePageLayout
{
  CGFloat const actionBarHeight = self.actionBar.height;
  self.height = self.basePlacePageView.height + self.anchorImageView.height + actionBarHeight - 1;
  self.actionBar.origin = CGPointMake(0., self.height - actionBarHeight);
  [self updatePlacePagePosition];
}

- (void)updatePlacePagePosition
{
  UIView * view = self.navigationController.view;
  view.maxY = self.availableHeight + kTopOffset;
  view.minY = MIN(view.minY, self.topBound + kTopOffset);
  [self configureContentInset];
}

- (void)configureContentInset
{
  UITableView * featureTable = self.basePlacePageView.featureTable;
  CGFloat const height = self.navigationController.view.height;
  CGFloat const tableContentHeight = featureTable.contentSize.height;
  CGFloat const headerViewHeight = self.basePlacePageView.separatorView.maxY;
  CGFloat const availableTableHeight = height - headerViewHeight - self.actionBar.height;
  CGFloat const externalHeight = tableContentHeight - availableTableHeight;
  if (externalHeight > 0.)
  {
    featureTable.contentInset = UIEdgeInsetsMake(0., 0., externalHeight, 0.);
    featureTable.scrollEnabled = YES;
  }
  else
  {
    [featureTable setContentOffset:CGPointZero animated:YES];
    featureTable.scrollEnabled = NO;
  }
}

- (CGFloat)availableHeight
{
  return self.parentViewHeight - self.keyboardHeight - kTopOffset - kBottomOffset;
}

#pragma mark - Properties

- (void)setHeight:(CGFloat)height
{
  _height = MIN(height, self.availableHeight);
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
  [UIView animateWithDuration:0.2f animations:^
  {
    self.navigationController.view.minY = topBound + kTopOffset;
  }];
}

@end
