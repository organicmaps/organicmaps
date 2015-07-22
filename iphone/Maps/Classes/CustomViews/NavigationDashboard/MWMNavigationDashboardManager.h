//
//  MWMNavigationDashboardManager.h
//  Maps
//
//  Created by v.mikhaylenko on 20.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

typedef NS_ENUM(NSUInteger, MWMNavigationRouteType)
{
  MWMNavigationRouteTypePedestrian,
  MWMNavigationRouteTypeVehicle
};

typedef NS_ENUM(NSUInteger, MWMNavigationDashboardState)
{
  MWMNavigationDashboardStateHidden,
  MWMNavigationDashboardStatePlanning,
  MWMNavigationDashboardStateNavigation
};

@protocol MWMNavigationDashboardManagerDelegate <NSObject>

- (void)buildRouteWithType:(enum MWMNavigationRouteType)type;

- (void)navigationDashBoardDidUpdate;

@end

@interface MWMNavigationDashboardManager : NSObject

@property (nonatomic) MWMNavigationDashboardState state;
@property (nonatomic) CGFloat topBound;
@property (nonatomic, readonly) CGFloat height;


- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentView:(UIView *)view delegate:(id<MWMNavigationDashboardManagerDelegate>)delegate;

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;

@end
