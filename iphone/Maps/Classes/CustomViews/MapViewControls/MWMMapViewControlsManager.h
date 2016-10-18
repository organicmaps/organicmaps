#import "MWMBottomMenuViewController.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMSearchManager.h"

#include "MWMRoutePoint.h"

#include "map/user_mark.hpp"
#include "platform/location.hpp"

@class MapViewController;
@class MWMPlacePageEntity;
@protocol MWMFeatureHolder;

@interface MWMMapViewControlsManager : NSObject

+ (MWMMapViewControlsManager *)manager;

@property(nonatomic) BOOL hidden;
@property(nonatomic) BOOL zoomHidden;
@property(nonatomic) BOOL sideButtonsHidden;
@property(nonatomic) MWMBottomMenuState menuState;
@property(nonatomic) MWMBottomMenuState menuRestoreState;
@property(nonatomic, readonly) MWMNavigationDashboardState navigationState;
@property(nonatomic) BOOL searchHidden;
@property(nonatomic) BOOL isDirectionViewHidden;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentController:(MapViewController *)controller;

- (UIStatusBarStyle)preferredStatusBarStyle;

#pragma mark - Layout

- (void)refreshLayout;
- (void)mwm_refreshUI;

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration;
- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator;

#pragma mark - MWMPlacePageViewManager

- (void)dismissPlacePage;
- (void)showPlacePage:(place_page::Info const &)info;
- (void)addPlacePageViews:(NSArray *)views;
- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point;
- (void)dragPlacePage:(CGRect)frame;

#pragma mark - MWMNavigationDashboardManager

- (void)onRoutePrepare;
- (void)onRouteRebuild;
- (void)onRouteError;
- (void)onRouteReady;
- (void)onRouteStart;
- (void)onRouteStop;

- (void)processMyPositionStateModeEvent:(location::EMyPositionMode)mode;

- (void)navigationDashBoardDidUpdate;

#pragma mark - MWMSearchManager

- (void)searchViewDidEnterState:(MWMSearchManagerState)state;
- (void)actionDownloadMaps:(mwm::DownloaderMode)mode;
- (void)searchFrameUpdated:(CGRect)frame;
- (void)searchText:(NSString *)text forInputLocale:(NSString *)locale;

#pragma mark - MWMFeatureHolder

- (id<MWMFeatureHolder>)featureHolder;

@end
