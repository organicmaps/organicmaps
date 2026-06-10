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
      _finish = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:points.back()
                                                              type:MWMRoutePointTypeFinish
                                                 intermediateIndex:0];
      _startRouteNavigation = parsedData.m_startRouteNavigation;
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
