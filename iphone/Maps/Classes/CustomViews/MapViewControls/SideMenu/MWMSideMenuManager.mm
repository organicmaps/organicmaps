//
//  MWMSideMenuManager.m
//  Maps
//
//  Created by Ilya Grechuhin on 24.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMSideMenuManager.h"
#import "MWMSideMenuButton.h"
#import "MWMSideMenuView.h"
#import "BookmarksRootVC.h"
#import "CountryTreeVC.h"
#import "SettingsAndMoreVC.h"
#import "MapsAppDelegate.h"
#import "LocationManager.h"
#import "ShareActionSheet.h"
#import "MapViewController.h"
#import "MWMMapViewControlsManager.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"

static NSString * const kMWMSideMenuViewsNibName = @"MWMSideMenuViews";

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMSideMenuManager()

@property (weak, nonatomic) MapViewController * parentController;
@property (nonatomic) IBOutlet MWMSideMenuButton * menuButton;
@property (nonatomic) IBOutlet MWMSideMenuView * sideMenu;

@end

@implementation MWMSideMenuManager

- (instancetype)initWithParentController:(MapViewController *)controller
{
  self = [super init];
  if (self)
  {
    self.parentController = controller;
    [[NSBundle mainBundle] loadNibNamed:kMWMSideMenuViewsNibName owner:self options:nil];
    [self addCloseMenuWithTap];
    self.state = MWMSideMenuStateInactive;
  }
  return self;
}

- (void)addCloseMenuWithTap
{
  UITapGestureRecognizer * const tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(toggleMenu)];
  [self.sideMenu.dimBackground addGestureRecognizer:tap];
}

#pragma mark - Actions

- (IBAction)menuActionOpenBookmarks
{
  self.state = MWMSideMenuStateInactive;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"bookmarks"];
  BookmarksRootVC * const vc = [[BookmarksRootVC alloc] init];
  [self.parentController.navigationController pushViewController:vc animated:YES];
}

- (IBAction)menuActionDownloadMaps
{
  self.state = MWMSideMenuStateInactive;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"downloader"];
  CountryTreeVC * const vc = [[CountryTreeVC alloc] initWithNodePosition:-1];
  [self.parentController.navigationController pushViewController:vc animated:YES];
}

- (IBAction)menuActionOpenSettings
{
  self.state = MWMSideMenuStateInactive;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"settingsAndMore"];
  SettingsAndMoreVC * const vc = [[SettingsAndMoreVC alloc] initWithStyle:UITableViewStyleGrouped];
  [self.parentController.navigationController pushViewController:vc animated:YES];
}

- (IBAction)menuActionShareLocation
{
  self.state = MWMSideMenuStateInactive;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"share@"];
  CLLocation const * const location = [MapsAppDelegate theApp].m_locationManager.lastLocation;
  if (!location)
  {
    [[[UIAlertView alloc] initWithTitle:L(@"unknown_current_position") message:nil delegate:nil cancelButtonTitle:L(@"ok") otherButtonTitles:nil] show];
    return;
  }
  CLLocationCoordinate2D const coord = location.coordinate;
  double const gX = MercatorBounds::LonToX(coord.longitude);
  double const gY = MercatorBounds::LatToY(coord.latitude);
  ShareInfo * const info = [[ShareInfo alloc] initWithText:nil gX:gX gY:gY myPosition:YES];
  self.parentController.shareActionSheet = [[ShareActionSheet alloc] initWithInfo:info viewController:self.parentController];
  UIView const * const parentView = self.parentController.view;
  [self.parentController.shareActionSheet showFromRect:CGRectMake(parentView.midX, parentView.height - 40.0, 0.0, 0.0)];
}

- (IBAction)menuActionOpenSearch
{
  self.parentController.controlsManager.hidden = YES;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"search"];
  [self.parentController.searchView setState:SearchViewStateFullscreen animated:YES withCallback:YES];
}

- (IBAction)toggleMenu
{
  if (self.state == MWMSideMenuStateActive)
    self.state = MWMSideMenuStateInactive;
  else if (self.state == MWMSideMenuStateInactive)
    self.state = MWMSideMenuStateActive;
}

#pragma mark - Properties

- (void)setState:(MWMSideMenuState)state
{
  _state = state;
  switch (state)
  {
    case MWMSideMenuStateActive:
      [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"menu"];
      [self.sideMenu addSelfToView:self.parentController.view];
      [self.menuButton removeFromSuperview];
      break;
    case MWMSideMenuStateInactive:
      [self.menuButton addSelfToView:self.parentController.view];
      [self.sideMenu removeFromSuperviewAnimated];
      break;
    case MWMSideMenuStateHidden:
      [self.menuButton addSelfHiddenToView:self.parentController.view];
      [self.sideMenu removeFromSuperview];
      break;
  }
}

@end
