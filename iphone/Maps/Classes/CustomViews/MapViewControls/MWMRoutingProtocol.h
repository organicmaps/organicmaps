#include "MWMRoutePoint.h"

@protocol MWMRoutingProtocol <NSObject>

- (void)buildRouteFrom:(MWMRoutePoint const &)from to:(MWMRoutePoint const &)to;
- (void)buildRouteTo:(MWMRoutePoint const &)to;
- (void)buildRouteFrom:(MWMRoutePoint const &)from;

@end
