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

@property (nonatomic) IBOutlet MWMRoutePreview * routePreview;
@property (nonatomic) IBOutlet MWMNavigationDashboard * navigationDashboard;

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
  }
  return self;
}

#pragma mark - MWMRoutePreview

- (IBAction)routePreviewChange:(UIButton *)sender
{
  enum MWMNavigationRouteType const type = [sender isEqual:self.routePreview.pedestrian] ? MWMNavigationRouteTypePedestrian : MWMNavigationRouteTypeVehicle;
  [self.delegate buildRouteWithType:type];
}

#pragma mark - MWMNavigationDashboard

- (IBAction)navigationCancelPressed:(UIButton *)sender
{
}

#pragma mark - MWMNavigationGo

- (IBAction)navigationGoPressed:(UIButton *)sender
{
  self.state = MWMNavigationDashboardStateNavigation;
}

#pragma mark - State changes

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

- (MWMRoutePreview *)routePreview
{
  if (!_routePreview)
    [NSBundle.mainBundle loadNibNamed:MWMRoutePreview.className owner:self options:nil];
  return _routePreview;
}

- (MWMNavigationDashboard *)navigationDashboard
{
  if (!_navigationDashboard)
    [NSBundle.mainBundle loadNibNamed:MWMNavigationDashboard.className owner:self options:nil];
  return _navigationDashboard;
}

- (void)setState:(MWMNavigationDashboardState)state
{
  if (_state == state)
    return;
  switch (state)
  {
    case MWMNavigationDashboardStateHidden:
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
