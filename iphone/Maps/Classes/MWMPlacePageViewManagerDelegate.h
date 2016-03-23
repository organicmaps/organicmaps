#import "MWMRoutingProtocol.h"

@protocol MWMPlacePageViewManagerProtocol <MWMRoutingProtocol>

- (void)dragPlacePage:(CGRect)frame;
- (void)addPlacePageViews:(NSArray *)views;
- (void)updateStatusBarStyle;
- (void)apiBack;
- (void)placePageDidClose;

@end