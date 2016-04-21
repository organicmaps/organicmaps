#import "MWMRoutingProtocol.h"

#include "geometry/point2d.hpp"

@protocol MWMPlacePageViewManagerProtocol <MWMRoutingProtocol>

- (void)dragPlacePage:(CGRect)frame;
- (void)addPlacePageViews:(NSArray *)views;
- (void)updateStatusBarStyle;
- (void)apiBack;
- (void)placePageDidClose;
- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point;

@end