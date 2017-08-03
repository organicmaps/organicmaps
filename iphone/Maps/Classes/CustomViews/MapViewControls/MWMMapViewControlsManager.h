#import "MWMBottomMenuViewController.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMSearchManager.h"

#include "map/place_page_info.hpp"

@class MapViewController;
@protocol MWMFeatureHolder;
@protocol MWMBookingInfoHolder;

@interface MWMMapViewControlsManager : NSObject

+ (MWMMapViewControlsManager *)manager;

@property(nonatomic) BOOL hidden;
@property(nonatomic) BOOL zoomHidden;
@property(nonatomic) BOOL sideButtonsHidden;
@property(nonatomic) BOOL trafficButtonHidden;
@property(nonatomic) MWMBottomMenuState menuState;
@property(nonatomic) MWMBottomMenuState menuRestoreState;
@property(nonatomic) BOOL isDirectionViewHidden;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentController:(MapViewController *)controller;

- (UIStatusBarStyle)preferredStatusBarStyle;

#pragma mark - Layout

- (void)mwm_refreshUI;

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator;

#pragma mark - MWMPlacePageViewManager

- (void)dismissPlacePage;
- (void)showPlacePage:(place_page::Info const &)info;
- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point;

#pragma mark - MWMNavigationDashboardManager

- (void)onRoutePrepare;
- (void)onRouteRebuild;
- (void)onRouteReady;
- (void)onRouteStart;
- (void)onRouteStop;

#pragma mark - MWMSearchManager

- (void)actionDownloadMaps:(mwm::DownloaderMode)mode;
- (BOOL)searchText:(NSString *)text forInputLocale:(NSString *)locale;
- (void)searchTextOnMap:(NSString *)text forInputLocale:(NSString *)locale;

#pragma mark - MWMFeatureHolder

- (id<MWMFeatureHolder>)featureHolder;

#pragma mark - MWMBookingInfoHolder

- (id<MWMBookingInfoHolder>)bookingInfoHolder;

@end
