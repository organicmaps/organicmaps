@protocol MWMPlacePageViewManagerProtocol <NSObject>

- (void)dragPlacePage:(CGRect)frame;
- (void)addPlacePageViews:(NSArray *)views;
- (void)updateStatusBarStyle;
- (void)buildRoute:(m2::PointD)destination;
- (void)apiBack;
- (void)placePageDidClose;

@end