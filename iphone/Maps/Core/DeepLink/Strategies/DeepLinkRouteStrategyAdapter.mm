#import "DeepLinkRouteStrategyAdapter.h"
#import <CoreApi/Framework.h>
#import "MWMCoreRouterType.h"

@implementation DeepLinkRouteStrategyAdapter

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    auto const parsedData = GetFramework().GetParsedRoutingData();
    _startRouteNavigation = parsedData.m_startRouteNavigation;
    _type = routerType(parsedData.m_type);
  }
  return self;
}

@end
