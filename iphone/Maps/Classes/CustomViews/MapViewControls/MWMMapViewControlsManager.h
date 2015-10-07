#import "MWMBottomMenuViewController.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMRoutingProtocol.h"

#include "map/user_mark.hpp"
#include "platform/location.hpp"

@class MapViewController;

@interface MWMMapViewControlsManager : NSObject <MWMRoutingProtocol>

@property (nonatomic) BOOL hidden;
@property (nonatomic) BOOL zoomHidden;
@property (nonatomic) MWMBottomMenuState menuState;
@property (nonatomic, readonly) MWMNavigationDashboardState navigationState;
@property (nonatomic) BOOL searchHidden;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentController:(MapViewController *)controller;

- (void)onEnterForeground;

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration;
- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator;

#pragma mark - MWMPlacePageViewManager

@property (nonatomic, readonly) BOOL isDirectionViewShown;

- (void)dismissPlacePage;
- (void)showPlacePageWithUserMark:(unique_ptr<UserMarkCopy>)userMark;

#pragma mark - MWMNavigationDashboardManager

- (void)setupRoutingDashboard:(location::FollowingInfo const &)info;
- (void)playTurnNotifications;
- (void)routingHidden;
- (void)routingReady;
- (void)routingPrepare;
- (void)routingNavigation;
- (void)handleRoutingError;
- (void)setRouteBuildingProgress:(CGFloat)progress;

@end
