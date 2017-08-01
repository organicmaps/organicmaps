#import "MWMBottomMenuView.h"
#import "MWMNavigationDashboardObserver.h"
#import "MWMTaxiPreviewDataSource.h"

typedef NS_ENUM(NSUInteger, MWMNavigationDashboardState) {
  MWMNavigationDashboardStateHidden,
  MWMNavigationDashboardStatePrepare,
  MWMNavigationDashboardStatePlanning,
  MWMNavigationDashboardStateError,
  MWMNavigationDashboardStateReady,
  MWMNavigationDashboardStateNavigation
};

@interface MWMNavigationDashboardManager : NSObject

+ (MWMNavigationDashboardManager *)manager;
+ (void)addObserver:(id<MWMNavigationDashboardObserver>)observer;
+ (void)removeObserver:(id<MWMNavigationDashboardObserver>)observer;

@property(nonatomic) MWMNavigationDashboardState state;
@property(nonatomic, readonly) MWMTaxiPreviewDataSource * taxiDataSource;
@property(nonatomic) NSString * errorMessage;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentView:(UIView *)view;
- (void)setRouteBuilderProgress:(CGFloat)progress;
- (void)mwm_refreshUI;

- (void)onRoutePointsUpdated;

+ (void)updateNavigationInfoAvailableArea:(CGRect)frame;

@end
