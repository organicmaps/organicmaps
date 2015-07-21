//
//  MWMNavigationDashboardManager.m
//  Maps
//
//  Created by v.mikhaylenko on 20.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Macros.h"
#import "MWMNavigationDashboard.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMRoutePreview.h"

@interface MWMNavigationDashboardManager ()

@property (nonatomic) IBOutlet MWMRoutePreview * routePreviewLandscape;
@property (nonatomic) IBOutlet MWMRoutePreview * routePreviewPortrait;
@property (weak, nonatomic) MWMRoutePreview * routePreview;


@property (nonatomic) IBOutlet MWMNavigationDashboard * navigationDashboardLandscape;
@property (nonatomic) IBOutlet MWMNavigationDashboard * navigationDashboardPortrait;
@property (weak, nonatomic) MWMNavigationDashboard * navigationDashboard;

@property (weak, nonatomic) UIView * ownerView;
@property (weak, nonatomic) id<MWMNavigationDashboardManagerDelegate> delegate;

@end

@implementation MWMNavigationDashboardManager

- (instancetype)initWithParentView:(UIView *)view delegate:(id<MWMNavigationDashboardManagerDelegate>)delegate
{
  self = [super init];
  if (self)
  {
    self.ownerView = view;
    self.delegate = delegate;
    BOOL const isPortrait = self.ownerView.width < self.ownerView.height;

    [NSBundle.mainBundle loadNibNamed:@"MWMPortraitRoutePreview" owner:self options:nil];
    [NSBundle.mainBundle loadNibNamed:@"MWMLandscapeRoutePreview" owner:self options:nil];
    self.routePreview = isPortrait ? self.routePreviewPortrait : self.routePreviewLandscape;

    [NSBundle.mainBundle loadNibNamed:@"MWMPortraitNavigationDashboard" owner:self options:nil];
    [NSBundle.mainBundle loadNibNamed:@"MWMLandscapeNavigationDashboard" owner:self options:nil];
    self.navigationDashboard = isPortrait ? self.navigationDashboardPortrait : self.navigationDashboardLandscape;
  }
  return self;
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  BOOL const isPortrait = orientation == UIInterfaceOrientationPortrait || orientation == UIInterfaceOrientationPortraitUpsideDown;
  MWMRoutePreview * routePreview = isPortrait ? self.routePreviewPortrait : self.routePreviewLandscape;
  if (self.routePreview.isVisible && ![routePreview isEqual:self.routePreview])
  {
    [self.routePreview remove];
    [routePreview addToView:self.ownerView];
  }
  self.routePreview = routePreview;

  MWMNavigationDashboard * navigationDashboard = isPortrait ? self.navigationDashboardPortrait : self.navigationDashboardLandscape;
  if (self.navigationDashboard.isVisible && ![navigationDashboard isEqual:self.navigationDashboard])
  {
    [self.navigationDashboard remove];
    [navigationDashboard addToView:self.ownerView];
  }
  self.navigationDashboard = navigationDashboard;
}

#pragma mark - MWMRoutePreview

- (IBAction)routePreviewChange:(UIButton *)sender
{
  self.routePreview.showGoButton = [sender isEqual:self.routePreview.pedestrian];
//  enum MWMNavigationRouteType const type = [sender isEqual:self.routePreview.pedestrian] ? MWMNavigationRouteTypePedestrian : MWMNavigationRouteTypeVehicle;
//  [self.delegate buildRouteWithType:type];
}

#pragma mark - MWMNavigationDashboard

- (IBAction)navigationCancelPressed:(UIButton *)sender
{
  self.state = MWMNavigationDashboardStateHidden;
}

#pragma mark - MWMNavigationGo

- (IBAction)navigationGoPressed:(UIButton *)sender
{
  self.state = MWMNavigationDashboardStateNavigation;
}

#pragma mark - State changes

- (void)hideState
{
  [self.routePreview remove];
  [self.navigationDashboard remove];
}

- (void)showStatePlanning
{
  [self.routePreview addToView:self.ownerView];
}

- (void)showStateNavigation
{
  [self.routePreview remove];
  [self.navigationDashboard addToView:self.ownerView];
}

#pragma mark - Properties

- (void)setState:(MWMNavigationDashboardState)state
{
  if (_state == state)
    return;
  switch (state)
  {
    case MWMNavigationDashboardStateHidden:
      [self hideState];
      break;
    case MWMNavigationDashboardStatePlanning:
      NSAssert(_state == MWMNavigationDashboardStateHidden, @"Invalid state change");
      [self showStatePlanning];
      break;
    case MWMNavigationDashboardStateNavigation:
      NSAssert(_state == MWMNavigationDashboardStatePlanning, @"Invalid state change");
      [self showStateNavigation];
      break;
  }
  _state = state;
}

@end
