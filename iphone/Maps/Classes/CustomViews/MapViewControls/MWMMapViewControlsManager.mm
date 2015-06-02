//
//  MWMMapViewControlsManager.m
//  Maps
//
//  Created by Ilya Grechuhin on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMMapViewControlsManager.h"
#import "MWMSideMenuManager.h"
#import "MWMZoomButtons.h"
#import "MWMLocationButton.h"
#import "MWMMapViewControlsWrapper.h"
#import "MapViewController.h"

#include "Framework.h"

@interface MWMMapViewControlsManager()

@property (nonatomic) MWMSideMenuManager * menuManager;
@property (nonatomic) MWMZoomButtons * zoomButtons;
@property (nonatomic) MWMLocationButton * locationButton;

@property (nonatomic) MWMMapViewControlsWrapper * viewWrapper;

@end

@implementation MWMMapViewControlsManager

- (instancetype)initWithParentController:(MapViewController *)controller
{
  if (!controller)
    return nil;
  self = [super init];
  if (!self)
    return nil;
  self.viewWrapper = [[MWMMapViewControlsWrapper alloc] initWithParentView:controller.view];
  if (!self.viewWrapper)
    return nil;
  self.menuManager = [[MWMSideMenuManager alloc] initWithParentView:self.viewWrapper andController:controller];
  self.zoomButtons = [[MWMZoomButtons alloc] initWithParentView:self.viewWrapper];
  self.locationButton = [[MWMLocationButton alloc] initWithParentView:self.viewWrapper];
  self.hidden = NO;
  self.zoomHidden = NO;
  self.menuHidden = NO;
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
  self.menuHidden = _menuHidden;
  self.locationHidden = _locationHidden;
  GetFramework().SetFullScreenMode(hidden);
}

- (void)setZoomHidden:(BOOL)zoomHidden
{
  _zoomHidden = zoomHidden;
  self.zoomButtons.hidden = self.hidden || zoomHidden;
}

- (void)setMenuHidden:(BOOL)menuHidden
{
  _menuHidden = menuHidden;
  self.menuManager.state = (self.hidden || menuHidden) ? MWMSideMenuStateHidden : MWMSideMenuStateInactive;
}

- (void)setLocationHidden:(BOOL)locationHidden
{
  _locationHidden = locationHidden;
  self.locationButton.hidden = self.hidden || locationHidden;
}

@end
