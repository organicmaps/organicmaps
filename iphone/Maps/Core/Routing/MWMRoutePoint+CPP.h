#import "MWMRoutePoint.h"

#include "map/mwm_url.hpp"
#include "map/routing_mark.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface MWMRoutePoint (CPP)

@property(nonatomic, readonly) RouteMarkData routeMarkData;

- (instancetype)initWithURLSchemeRoutePoint:(url_scheme::RoutePoint const &)point
                                       type:(MWMRoutePointType)type
                          intermediateIndex:(size_t)intermediateIndex;
- (instancetype)initWithRouteMarkData:(RouteMarkData const &)point;
- (instancetype)initWithPoint:(m2::PointD const &)point
                        title:(nullable NSString *)title
                     subtitle:(nullable NSString *)subtitle
                         type:(MWMRoutePointType)type
            intermediateIndex:(size_t)intermediateIndex;

@end

NS_ASSUME_NONNULL_END
