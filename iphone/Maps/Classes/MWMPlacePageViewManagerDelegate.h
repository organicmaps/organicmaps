#import "MWMRoutingProtocol.h"

#include "geometry/point2d.hpp"

@protocol MWMPlacePageViewManagerProtocol <MWMRoutingProtocol>

- (void)dragPlacePage:(CGRect)frame;
- (void)addPlacePageViews:(NSArray *)views;
- (void)updateStatusBarStyle;
- (void)apiBack;
- (void)placePageDidClose;
- (void)addBusiness;
- (void)addPlace;

@end