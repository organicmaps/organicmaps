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

- (void)didStartEditingRoutePoint:(BOOL)isSource;
- (void)setMenuState:(MWMBottomMenuState)menuState;

@end

@interface MWMNavigationDashboardManager : NSObject <MWMLocationObserver>

+ (MWMNavigationDashboardManager *)manager;

@property (nonatomic, readonly) MWMNavigationDashboardEntity * entity;
@property (nonatomic, readonly) MWMRoutePreview * routePreview;
@property (nonatomic) MWMNavigationDashboardState state;
@property (weak, nonatomic, readonly) id<MWMNavigationDashboardManagerProtocol> delegate;
@property (nonatomic) CGFloat topBound;
@property (nonatomic) CGFloat leftBound;
@property (nonatomic, readonly) CGFloat height;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentView:(UIView *)view infoDisplay:(id<MWMNavigationDashboardInfoProtocol>)infoDisplay delegate:(id<MWMNavigationDashboardManagerProtocol>)delegate;
- (void)updateFollowingInfo:(location::FollowingInfo const &)info;
- (void)updateDashboard;
- (void)setRouteBuilderProgress:(CGFloat)progress;
- (void)mwm_refreshUI;

@end
