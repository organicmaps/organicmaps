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

@protocol MWMNavigationDashboardManagerDelegate <NSObject>

- (void)buildRouteWithType:(enum MWMNavigationRouteType)type;

@end

@interface MWMNavigationDashboardManager : NSObject

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentView:(UIView *)view delegate:(id<MWMNavigationDashboardManagerDelegate>)delegate;

@end
