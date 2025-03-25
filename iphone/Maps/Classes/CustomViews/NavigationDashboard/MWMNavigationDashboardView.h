#import "RoutePreviewView.h"
#import "SearchOnMapState.h"

NS_ASSUME_NONNULL_BEGIN

@class MWMNavigationDashboardEntity;

@protocol NavigationDashboardView

@property(weak, nonatomic) id<MWMRoutePreviewDelegate> delegate;

- (void)onNavigationInfoUpdated:(MWMNavigationDashboardEntity *)entity;
- (void)setDrivingOptionState:(MWMDrivingOptionsState)state;
- (void)searchManagerWithDidChangeState:(SearchOnMapState)state;
- (void)updateNavigationInfoAvailableArea:(CGRect)frame;
- (void)setRouteBuilderProgress:(MWMRouterType)router progress:(CGFloat)progress;

- (void)setHidden:(BOOL)hidden;
- (void)stateClosed;
- (void)statePrepare;
- (void)statePlanning;
- (void)stateError:(NSString *_Nonnull)errorMessage;
- (void)stateReady;
- (void)onRouteStart;
- (void)onRouteStop;
- (void)onRoutePointsUpdated;
- (void)stateNavigation;

@end

NS_ASSUME_NONNULL_END
