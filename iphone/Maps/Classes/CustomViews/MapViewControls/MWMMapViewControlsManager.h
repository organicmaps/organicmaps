//
//  MWMMapViewControlsManager.h
//  Maps
//
//  Created by Ilya Grechuhin on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMNavigationDashboardManager.h"
#import "MWMSideMenuManager.h"

#include "map/user_mark.hpp"
#include "platform/location.hpp"

@class MapViewController;

@protocol MWMMapViewControlsManagerDelegate <NSObject>

- (void)addPlacePageViews:(NSArray *)views;

@end

@interface MWMMapViewControlsManager : NSObject

@property (nonatomic) BOOL hidden;
@property (nonatomic) BOOL zoomHidden;
@property (nonatomic) BOOL locationHidden;
@property (nonatomic) MWMSideMenuState menuState;
@property (nonatomic, readonly) MWMNavigationDashboardState navigationState;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentController:(MapViewController *)controller;

#pragma mark - Layout

@property (nonatomic) CGFloat topBound;

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;

#pragma mark - MWMPlacePageViewManager

@property (nonatomic, readonly) BOOL isDirectionViewShown;

- (void)dismissPlacePage;
- (void)showPlacePageWithUserMark:(unique_ptr<UserMarkCopy>)userMark;

#pragma mark - MWMNavigationDashboardManager

- (void)setupRoutingDashboard:(location::FollowingInfo const &)info;
- (void)routingReady;
- (void)routingNavigation;
- (void)handleRoutingError;

@end
