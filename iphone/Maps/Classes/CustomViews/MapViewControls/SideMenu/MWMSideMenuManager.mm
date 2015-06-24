//
//  MWMSideMenuManager.m
//  Maps
//
//  Created by Ilya Grechuhin on 24.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "BookmarksRootVC.h"
#import "CountryTreeVC.h"
#import "Framework.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMMapViewControlsCommon.h"
#import "MWMMapViewControlsManager.h"
#import "MWMSideMenuButton.h"
#import "MWMSideMenuDelegate.h"
#import "MWMSideMenuManager.h"
#import "MWMSideMenuView.h"
#import "SettingsAndMoreVC.h"
#import "ShareActionSheet.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "map/information_display.hpp"

static NSString * const kMWMSideMenuViewsNibName = @"MWMSideMenuViews";

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMSideMenuManager() <MWMSideMenuInformationDisplayProtocol>

@property (weak, nonatomic) MapViewController * controller;
@property (nonatomic) IBOutlet MWMSideMenuButton * menuButton;
@property (nonatomic) IBOutlet MWMSideMenuView * sideMenu;

@end

@implementation MWMSideMenuManager

- (instancetype)initWithParentController:(MapViewController *)controller
{
  self = [super init];
  if (self)
  {
    self.controller = controller;
    [[NSBundle mainBundle] loadNibNamed:kMWMSideMenuViewsNibName owner:self options:nil];
    [self.controller.view addSubview:self.menuButton];
    [self.menuButton setup];
    self.menuButton.delegate = self;
    self.sideMenu.delegate = self;
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
  [self.controller.navigationController pushViewController:vc animated:YES];
}

- (IBAction)menuActionDownloadMaps
{
  self.state = MWMSideMenuStateInactive;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"downloader"];
  CountryTreeVC * const vc = [[CountryTreeVC alloc] initWithNodePosition:-1];
  [self.controller.navigationController pushViewController:vc animated:YES];
}

- (IBAction)menuActionOpenSettings
{
  self.state = MWMSideMenuStateInactive;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"settingsAndMore"];
  SettingsAndMoreVC * const vc = [[SettingsAndMoreVC alloc] initWithStyle:UITableViewStyleGrouped];
  [self.controller.navigationController pushViewController:vc animated:YES];
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
  self.controller.shareActionSheet = [[ShareActionSheet alloc] initWithInfo:info viewController:self.controller];
  UIView const * const parentView = self.controller.view;
  [self.controller.shareActionSheet showFromRect:CGRectMake(parentView.midX, parentView.height - 40.0, 0.0, 0.0)];
}

- (IBAction)menuActionOpenSearch
{
  self.controller.controlsManager.hidden = YES;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"search"];
  [self.controller.searchView setState:SearchViewStateFullscreen animated:YES];
}

- (IBAction)toggleMenu
{
  if (self.state == MWMSideMenuStateActive)
    self.state = MWMSideMenuStateInactive;
  else if (self.state == MWMSideMenuStateInactive)
    self.state = MWMSideMenuStateActive;
}

#pragma mark - MWMSideMenuInformationDisplayProtocol

- (void)setRulerPivot:(m2::PointD)pivot
{
  // Workaround for old ios when layoutSubviews are called in undefined order.
  dispatch_async(dispatch_get_main_queue(), ^
  {
    GetFramework().GetInformationDisplay().SetWidgetPivot(InformationDisplay::WidgetType::Ruler, pivot);
  });
}

- (void)setCopyrightLabelPivot:(m2::PointD)pivot
{
  // Workaround for old ios when layoutSubviews are called in undefined order.
  dispatch_async(dispatch_get_main_queue(), ^
  {
    GetFramework().GetInformationDisplay().SetWidgetPivot(InformationDisplay::WidgetType::CopyrightLabel, pivot);
  });
}

- (void)showMenu
{
  self.menuButton.alpha = 1.0;
  self.sideMenu.alpha = 0.0;
  [self.controller.view addSubview:self.sideMenu];
  [UIView animateWithDuration:framesDuration(3) animations:^
  {
    self.menuButton.alpha = 0.0;
    self.sideMenu.alpha = 1.0;
  }
  completion:^(BOOL finished)
  {
    [self.menuButton setHidden:YES animated:NO];
  }];
}

- (void)hideMenu
{
  [UIView animateWithDuration:framesDuration(3) animations:^
  {
    self.menuButton.alpha = 1.0;
    self.sideMenu.alpha = 0.0;
  }
  completion:^(BOOL finished)
  {
    [self.sideMenu removeFromSuperview];
  }];
}

#pragma mark - Properties

- (void)setState:(MWMSideMenuState)state
{
  if (_state == state)
    return;
  switch (state)
  {
    case MWMSideMenuStateActive:
      [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"menu"];
      self.sideMenu.outOfDateCount = GetFramework().GetCountryTree().GetActiveMapLayout().GetOutOfDateCount();
      [self showMenu];
      [self.sideMenu setup];
      break;
    case MWMSideMenuStateInactive:
      if (_state == MWMSideMenuStateActive)
      {
        [self.menuButton setHidden:NO animated:NO];
        [self hideMenu];
      }
      else
      {
        [self.menuButton setHidden:NO animated:YES];
      }
      break;
    case MWMSideMenuStateHidden:
      [self.menuButton setHidden:YES animated:YES];
      [self hideMenu];
      break;
  }
  _state = state;
  [self.controller updateStatusBarStyle];
}

@end
