typedef NS_ENUM(NSUInteger, MWMNavigationDashboardState) {
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

- (instancetype _Nonnull)init __attribute__((unavailable("init is not available")));
- (instancetype _Nonnull)initWithParentView:(UIView *_Nonnull)view;
- (void)setRouteBuilderProgress:(CGFloat)progress;

- (void)onRoutePrepare;
- (void)onRoutePlanning;
- (void)onRouteError:(NSString *_Nonnull)error;
- (void)onRouteReady:(BOOL)hasWarnings;
- (void)onRouteStart;
- (void)onRouteStop;
- (void)onRoutePointsUpdated;

+ (void)updateNavigationInfoAvailableArea:(CGRect)frame;

@end
