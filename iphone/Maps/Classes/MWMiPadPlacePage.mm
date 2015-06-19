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

@interface MWMiPadNavigationController : UINavigationController

@end

@implementation MWMiPadNavigationController

- (instancetype)initWithRootViewController:(UIViewController *)rootViewController
{
  self = [super initWithRootViewController:rootViewController];
  if (self)
  {
    [self setNavigationBarHidden:YES];
    self.view.autoresizingMask = UIViewAutoresizingNone;
  }
  return self;
}

- (void)backTap:(id)sender
{
  [self popViewControllerAnimated:YES];
}

@end

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

@interface MWMiPadPlacePage ()

@property (strong, nonatomic) MWMiPadNavigationController * navigationController;
@property (strong, nonatomic) MWMiPadPlacePageViewController * viewController;

@end

@implementation MWMiPadPlacePage

- (void)configure
{
  [super configure];
  self.viewController = [[MWMiPadPlacePageViewController alloc] init];
  [self.navigationController.view removeFromSuperview];
  [self.navigationController removeFromParentViewController];
  self.navigationController = [[MWMiPadNavigationController alloc] initWithRootViewController:self.viewController];

  UIView * view = self.manager.ownerViewController.view;
  CGFloat const topOffset = 36.;
  CGFloat const leftOffset = 12.;
  CGFloat const defaultWidth = 360.;
  CGFloat const actionBarHeight = self.actionBar.height;
  CGFloat const height = self.basePlacePageView.height + self.anchorImageView.height + actionBarHeight - 1;

  self.navigationController.view.frame = CGRectMake(leftOffset, topOffset, defaultWidth, height);
  self.viewController.view.frame = self.navigationController.view.frame;
  [self.manager.ownerViewController addChildViewController:self.navigationController];
  [view addSubview:self.navigationController.view];
  [self.viewController.view addSubview:self.extendedPlacePageView];
  self.extendedPlacePageView.size = self.navigationController.view.size;
  [self.viewController.view addSubview:self.extendedPlacePageView];
  self.extendedPlacePageView.origin = CGPointZero;
  self.anchorImageView.image = nil;
  self.anchorImageView.backgroundColor = [UIColor whiteColor];
  [self.extendedPlacePageView addSubview:self.actionBar];
  self.actionBar.frame = CGRectMake(0., self.extendedPlacePageView.maxY - actionBarHeight, defaultWidth, actionBarHeight);
}

- (void)dismiss
{
  [self.navigationController.view removeFromSuperview];
  [self.navigationController removeFromParentViewController];
  [super dismiss];
}

- (void)addBookmark
{
  [super addBookmark];
  self.navigationController.view.height += kBookmarkCellHeight;
  self.viewController.view.height += kBookmarkCellHeight;
  self.extendedPlacePageView.height += kBookmarkCellHeight;
  self.actionBar.minY += kBookmarkCellHeight;
}

- (void)removeBookmark
{
  [super removeBookmark];
  self.navigationController.view.height -= kBookmarkCellHeight;
  self.viewController.view.height -= kBookmarkCellHeight;
  self.extendedPlacePageView.height -= kBookmarkCellHeight;
  self.actionBar.minY -= kBookmarkCellHeight;
}

- (void)changeBookmarkColor
{
  MWMBookmarkColorViewController * controller = [[MWMBookmarkColorViewController alloc] initWithNibName:[MWMBookmarkColorViewController className] bundle:nil];
  controller.iPadOwnerNavigationController = self.navigationController;
  controller.placePageManager = self.manager;
  controller.view.frame = self.viewController.view.frame;
  [self.viewController.navigationController pushViewController:controller animated:YES];
}

- (void)changeBookmarkCategory
{
  SelectSetVC * controller = [[SelectSetVC alloc] initWithPlacePageManager:self.manager];
  controller.iPadOwnerNavigationController = self.navigationController;
  controller.view.frame = self.viewController.view.frame;
  [self.viewController.navigationController pushViewController:controller animated:YES];
}

- (void)changeBookmarkDescription
{
  MWMBookmarkDescriptionViewController * controller = [[MWMBookmarkDescriptionViewController alloc] initWithPlacePageManager:self.manager];
  controller.iPadOwnerNavigationController = self.navigationController;
  [self.viewController.navigationController pushViewController:controller animated:YES];
}

@end
