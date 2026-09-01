#import "MWMRoutePoint.h"

typedef NS_ENUM(NSUInteger, MWMNavigationDashboardState) {
  MWMNavigationDashboardStateClosed,
  MWMNavigationDashboardStateHidden,
  MWMNavigationDashboardStatePrepare,
  MWMNavigationDashboardStatePlanning,
  MWMNavigationDashboardStateError,
  MWMNavigationDashboardStateReady,
  MWMNavigationDashboardStateNavigation
};

@interface MWMNavigationDashboardManager : NSObject

+ (nonnull MWMNavigationDashboardManager *)sharedManager;

@property(nonatomic, readonly) MWMNavigationDashboardState state;
@property(weak, nonatomic, readonly, nullable) UIView * availableAreaView;
@property(nonatomic, readonly, nullable) MWMRoutePoint * selectedRoutePoint;
@property(nonatomic, readonly) BOOL shouldAppendNewPoints;
@property(nonatomic, readonly) BOOL isRoutePointSelectionActive;
@property(nonatomic, readonly) NSString * _Nonnull routePointSelectionTitle;
@property(nonatomic, readonly) BOOL canSelectCurrentLocation;

- (instancetype _Nonnull)init __attribute__((unavailable("init is not available")));
- (instancetype _Nonnull)initWithParentViewController:(UIViewController * _Nonnull)viewController;
- (void)setRouteBuilderProgress:(CGFloat)progress;

- (void)onSelectPlacePage:(BOOL)selected;
- (void)onRoutePrepare;
- (void)onRoutePlanning;
- (void)onRouteError:(NSString * _Nonnull)error;
- (void)onRouteReady:(BOOL)hasWarnings;
- (void)onRouteStart;
- (void)onRouteStop;
- (void)onRoutePointsUpdated;

- (BOOL)selectCurrentLocationForRoute;
- (BOOL)selectRoutePointAtPoint:(CGPoint)point
                          title:(nullable NSString *)title
                       subtitle:(nullable NSString *)subtitle NS_SWIFT_NAME(selectRoutePoint(at:title:subtitle:));
- (void)cancelRoutePointSelection;

+ (void)updateNavigationInfoAvailableArea:(CGRect)frame;

@end
