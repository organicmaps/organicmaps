#import "MWMBottomMenuView.h"
#import "MWMCircularProgress.h"
#import "MWMLocationManager.h"
#import "MWMNavigationDashboardEntity.h"
#import "MWMNavigationViewProtocol.h"
#import "MWMRoutePreview.h"

#include "Framework.h"
#include "platform/location.hpp"

typedef NS_ENUM(NSUInteger, MWMNavigationDashboardState)
{
  MWMNavigationDashboardStateHidden,
  MWMNavigationDashboardStatePrepare,
  MWMNavigationDashboardStatePlanning,
  MWMNavigationDashboardStateError,
  MWMNavigationDashboardStateReady,
  MWMNavigationDashboardStateNavigation
};

@protocol MWMNavigationDashboardInfoProtocol

- (void)updateRoutingInfo:(MWMNavigationDashboardEntity *)info;

@end

@protocol MWMNavigationDashboardManagerProtocol <MWMNavigationViewProtocol>

- (void)buildRoute;
- (BOOL)isPossibleToBuildRoute;
- (BOOL)didStartRouting;
- (void)didCancelRouting;
- (void)updateStatusBarStyle;
- (void)didStartEditingRoutePoint:(BOOL)isSource;
- (void)swapPointsAndRebuildRouteIfPossible;

- (void)setMenuState:(MWMBottomMenuState)menuState;

@end

@interface MWMNavigationDashboardManager : NSObject <MWMLocationObserver>

@property (nonatomic, readonly) MWMNavigationDashboardEntity * entity;
@property (weak, nonatomic, readonly) MWMRoutePreview * routePreview;
@property (nonatomic) MWMNavigationDashboardState state;
@property (weak, nonatomic, readonly) id<MWMNavigationDashboardManagerProtocol> delegate;
@property (nonatomic) CGFloat topBound;
@property (nonatomic) CGFloat leftBound;
@property (nonatomic, readonly) CGFloat height;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentView:(UIView *)view infoDisplay:(id<MWMNavigationDashboardInfoProtocol>)infoDisplay delegate:(id<MWMNavigationDashboardManagerProtocol, MWMRoutePreviewDataSource>)delegate;
- (void)updateFollowingInfo:(location::FollowingInfo const &)info;
- (void)updateDashboard;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;
- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator;
- (void)setActiveRouter:(routing::RouterType)routerType;
- (void)setRouteBuilderProgress:(CGFloat)progress;
- (void)showHelperPanels;
- (void)hideHelperPanels;
- (void)setupActualRoute;
- (void)mwm_refreshUI;

@end
