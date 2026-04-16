typedef NS_ENUM(NSUInteger, MWMNavigationDashboardState) {
  MWMNavigationDashboardStateClosed,
  MWMNavigationDashboardStateHidden,
  MWMNavigationDashboardStatePrepare,
  MWMNavigationDashboardStatePlanning,
  MWMNavigationDashboardStateError,
  MWMNavigationDashboardStateReady,
  MWMNavigationDashboardStateNavigation
};

@class MWMRoutePoint;

@interface MWMNavigationDashboardManager : NSObject

+ (nonnull MWMNavigationDashboardManager *)sharedManager;

@property(nonatomic, readonly) MWMNavigationDashboardState state;
@property(weak, nonatomic, readonly, nullable) UIView * availableAreaView;
@property(nonatomic, readonly, nullable) MWMRoutePoint * selectedRoutePoint;
@property(nonatomic, readonly) BOOL shouldAppendNewPoints;

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

+ (void)updateNavigationInfoAvailableArea:(CGRect)frame;

@end
