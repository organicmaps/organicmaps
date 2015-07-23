//
//  MWMNavigationDashboardManager.h
//  Maps
//
//  Created by v.mikhaylenko on 20.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#include "Framework.h"
#include "platform/location.hpp"

typedef NS_ENUM(NSUInteger, MWMNavigationDashboardState)
{
  MWMNavigationDashboardStateHidden,
  MWMNavigationDashboardStatePlanning,
  MWMNavigationDashboardStateReady,
  MWMNavigationDashboardStateNavigation
};

@protocol MWMNavigationDashboardManagerDelegate <NSObject>

- (void)buildRouteWithType:(enum routing::RouterType)type;
- (void)navigationDashBoardDidUpdate;
- (void)didStartFollowing;
- (void)didCancelRouting;

@end

@class MWMNavigationDashboardEntity;

@interface MWMNavigationDashboardManager : NSObject

@property (nonatomic, readonly) MWMNavigationDashboardEntity * entity;
@property (nonatomic) MWMNavigationDashboardState state;
@property (nonatomic) CGFloat topBound;
@property (nonatomic, readonly) CGFloat height;


- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentView:(UIView *)view delegate:(id<MWMNavigationDashboardManagerDelegate>)delegate;
- (void)setupDashboard:(location::FollowingInfo const &)info;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;

@end
