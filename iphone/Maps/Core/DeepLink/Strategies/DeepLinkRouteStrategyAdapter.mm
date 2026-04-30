#import "DeepLinkRouteStrategyAdapter.h"
#import <CoreApi/Framework.h>
#import "MWMCoreRouterType.h"
#import "MWMRoutePoint+CPP.h"

@implementation DeepLinkRouteStrategyAdapter

- (instancetype)init:(NSURL *)url
{
  self = [super init];
  if (self)
  {
    auto const parsedData = GetFramework().GetParsedRoutingData();
    auto const points = parsedData.m_points;

    if (points.size() >= 2)
    {
      _start = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:points.front()
                                                             type:MWMRoutePointTypeStart
                                                intermediateIndex:0];
      // Preserve the URL order: the first point is the start, the last point is
      // the finish, and every point in between is an explicitly requested stop.
      NSMutableArray<MWMRoutePoint *> * intermediatePoints = [NSMutableArray arrayWithCapacity:points.size() - 2];
      for (size_t i = 1; i + 1 < points.size(); ++i)
      {
        size_t const intermediateIndex = parsedData.m_optimizeRoutePoints ? 0 : i - 1;
        [intermediatePoints addObject:[[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:points[i]
                                                                                    type:MWMRoutePointTypeIntermediate
                                                                       intermediateIndex:intermediateIndex]];
      }
      _intermediatePoints = [intermediatePoints copy];
      _finish = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:points.back()
                                                              type:MWMRoutePointTypeFinish
                                                 intermediateIndex:0];
      _optimizeRoutePoints = parsedData.m_optimizeRoutePoints;
      _startRouteNavigation = parsedData.m_startRouteNavigation;
      _startDirection = CGPointMake(parsedData.m_startDirection.x, parsedData.m_startDirection.y);
      _type = routerType(parsedData.m_type);
    }
    else
    {
      return nil;
    }
  }
  return self;
}

@end
