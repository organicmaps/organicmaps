//
//  MWMNavigationDashboardManager.m
//  Maps
//
//  Created by v.mikhaylenko on 20.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Macros.h"
#import "MWMiPadLandscapeRoutePreview.h"
#import "MWMNavigationDashboard.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMNavigationGo.h"
#import "MWMRoutePreview.h"

@interface MWMNavigationDashboardManager ()

@property (nonatomic) IBOutlet MWMRoutePreview * routePreview;
@property (nonatomic) IBOutlet MWMiPadLandscapeRoutePreview * iPadLandscapeRoutePreview;
@property (nonatomic) IBOutlet MWMNavigationDashboard * navigationDashboard;
@property (nonatomic) IBOutlet MWMNavigationGo * navigatonGo;

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

- (IBAction)navigationGoPressed:(MWMNavigationGo *)sender
{
}

#pragma mark - Properties

- (MWMRoutePreview *)routePreview
{
  if (!_routePreview)
    [NSBundle.mainBundle loadNibNamed:MWMRoutePreview.className owner:self options:nil];
  return _routePreview;
}

- (MWMiPadLandscapeRoutePreview *)iPadLandscapeRoutePreview
{
  if (!_iPadLandscapeRoutePreview)
    [NSBundle.mainBundle loadNibNamed:MWMiPadLandscapeRoutePreview.className owner:self options:nil];
  return _iPadLandscapeRoutePreview;
}

- (MWMNavigationDashboard *)navigationDashboard
{
  if (!_navigationDashboard)
    [NSBundle.mainBundle loadNibNamed:MWMNavigationDashboard.className owner:self options:nil];
  return _navigationDashboard;
}

- (MWMNavigationGo *)navigatonGo
{
  if (!_navigatonGo)
    [NSBundle.mainBundle loadNibNamed:MWMNavigationGo.className owner:self options:nil];
  return _navigatonGo;
}

- (void)setState:(MWMNavigationDashboardState)state
{
  if (_state == state)
    return;
  _state = state;
  switch (state)
  {
    case MWMNavigationDashboardStateHidden:
      break;
    case MWMNavigationDashboardStatePlanning:
      [self.ownerView addSubview:self.routePreview];
      break;
  }
}

@end
