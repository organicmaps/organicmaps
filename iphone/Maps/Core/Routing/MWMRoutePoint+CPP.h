#import "MWMRoutePoint.h"

#include "map/routing_mark.hpp"

@interface MWMRoutePoint (CPP)

@property(nonatomic, readonly) RouteMarkData routeMarkData;

- (instancetype)initWithRouteMarkData:(RouteMarkData const &)point;
- (instancetype)initWithPoint:(m2::PointD const &)point
                        title:(NSString *)title
                     subtitle:(NSString *)subtitle
                         type:(MWMRoutePointType)type
            intermediateIndex:(size_t)intermediateIndex;

@end
