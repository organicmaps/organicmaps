//
//  MWMMapViewControlsManager.m
//  Maps
//
//  Created by Ilya Grechuhin on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MapViewController.h"
#import "MWMLocationButton.h"
#import "MWMMapViewControlsManager.h"
#import "MWMSideMenuManager.h"
#import "MWMZoomButtons.h"

#include "Framework.h"

@interface MWMMapViewControlsManager()

@property (nonatomic) MWMZoomButtons * zoomButtons;
@property (nonatomic) MWMLocationButton * locationButton;
@property (nonatomic) MWMSideMenuManager * menuManager;

@end

@implementation MWMMapViewControlsManager

- (instancetype)initWithParentController:(MapViewController *)controller
{
  if (!controller)
    return nil;
  self = [super init];
  if (!self)
    return nil;
  self.zoomButtons = [[MWMZoomButtons alloc] initWithParentView:controller.view];
  self.locationButton = [[MWMLocationButton alloc] initWithParentView:controller.view];
  self.menuManager = [[MWMSideMenuManager alloc] initWithParentController:controller];
  self.hidden = NO;
  self.zoomHidden = NO;
  self.menuState = MWMSideMenuStateInactive;
  return self;
}

- (void)resetZoomButtonsVisibility
{
  [self.zoomButtons resetVisibility];
}

- (void)moveButton:(MWMMapViewControlsButton)button toDefaultPosition:(BOOL)defaultPosition
{
  switch (button)
  {
    case MWMMapViewControlsButtonZoom:
      [self.zoomButtons moveToDefaultPosition:defaultPosition];
      break;
    case MWMMapViewControlsButtonMenu:
    case MWMMapViewControlsButtonLocation:
      break;
  }
}

- (void)setBottomBound:(CGFloat)bound
{
  [self.zoomButtons setBottomBound:bound];
}

#pragma mark - Properties

- (void)setHidden:(BOOL)hidden
{
  if (_hidden == hidden)
    return;
  _hidden = hidden;
  self.zoomHidden = _zoomHidden;
  self.menuState = self.menuManager.state;
  self.locationHidden = _locationHidden;
  GetFramework().SetFullScreenMode(hidden);
}

- (void)setZoomHidden:(BOOL)zoomHidden
{
  _zoomHidden = zoomHidden;
  self.zoomButtons.hidden = self.hidden || zoomHidden;
}

- (void)setMenuState:(MWMSideMenuState)menuState
{
  self.menuManager.state = self.hidden ? MWMSideMenuStateHidden : menuState;
}

- (MWMSideMenuState)menuState
{
  return self.menuManager.state;
}

- (void)setLocationHidden:(BOOL)locationHidden
{
  _locationHidden = locationHidden;
  self.locationButton.hidden = self.hidden || locationHidden;
}

@end
