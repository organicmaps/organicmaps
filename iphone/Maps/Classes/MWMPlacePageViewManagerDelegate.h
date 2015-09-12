@protocol MWMPlacePageViewManagerProtocol <NSObject>

- (void)dragPlacePage:(CGPoint)point;
- (void)addPlacePageViews:(NSArray *)views;
- (void)updateStatusBarStyle;
- (void)buildRoute:(m2::PointD)destination;
- (void)apiBack;
- (void)placePageDidClose;

@end