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
#import "MWMNavigationGo.h"
#import "MWMRoutePreview.h"

@interface MWMNavigationDashboardManager ()

@property (nonatomic) IBOutlet MWMRoutePreview * routePreview;
@property (nonatomic) IBOutlet MWMNavigationDashboard * navigationDashboard;
@property (nonatomic) IBOutlet MWMNavigationGo * navigatonGo;

@end

@implementation MWMNavigationDashboardManager

#pragma mark - MWMRoutePreview

- (IBAction)routePreviewChange:(UIButton *)sender
{
  if ([sender isEqual:self.routePreview.walk])
  {
    // Build walk route
  }
  else
  {
    // Build drive route
  }
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
    [[NSBundle mainBundle] loadNibNamed:MWMRoutePreview.className owner:self options:nil];
  return _routePreview;
}

- (MWMNavigationDashboard *)navigationDashboard
{
  if (!_navigationDashboard)
    [[NSBundle mainBundle] loadNibNamed:MWMNavigationDashboard.className owner:self options:nil];
  return _navigationDashboard;
}

- (MWMNavigationGo *)navigatonGo
{
  if (!_navigatonGo)
    [[NSBundle mainBundle] loadNibNamed:MWMNavigationGo.className owner:self options:nil];
  return _navigatonGo;
}

@end
