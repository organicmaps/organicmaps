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
#import "MapViewController.h"

@interface MWMMapViewControlsManager()

@property (nonatomic) MWMSideMenuManager * menuManager;
@property (nonatomic) MWMZoomButtons * zoomButtons;
@property (nonatomic) MWMLocationButton * locationButton;

@end

@implementation MWMMapViewControlsManager

- (instancetype)initWithParentController:(MapViewController *)controller
{
  self = [super init];
  if (self && controller)
  {
    self.menuManager = [[MWMSideMenuManager alloc] initWithParentController:controller];
    self.zoomButtons = [[MWMZoomButtons alloc] initWithParentView:controller.view];
    self.locationButton = [[MWMLocationButton alloc] initWithParentView:controller.view];
    self.hidden = NO;
    self.zoomHidden = NO;
    self.menuHidden = NO;
  }
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
    default:
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
  if (_hidden != hidden)
  {
    _hidden = hidden;
    self.zoomHidden = _zoomHidden;
    self.menuHidden = _menuHidden;
    self.locationHidden = _locationHidden;
  }
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
