#import "MWMRouter.h"

#include <CoreApi/Framework.h>

@interface MWMRouter ()

@property(nonatomic) uint32_t routeManagerTransactionId;

+ (MWMRouter *)router;

@end

@implementation MWMRouter (RouteManager)

+ (void)openRouteManagerTransaction
{
  auto router = [MWMRouter router];
  router.routeManagerTransactionId = GetFramework().GetRoutingManager().OpenRoutePointsTransaction();
}

+ (void)applyRouteManagerTransaction
{
  auto router = [MWMRouter router];
  if (router.routeManagerTransactionId == RoutingManager::InvalidRoutePointsTransactionId())
    return;
  GetFramework().GetRoutingManager().ApplyRoutePointsTransaction(router.routeManagerTransactionId);
  router.routeManagerTransactionId = RoutingManager::InvalidRoutePointsTransactionId();
}

+ (void)cancelRouteManagerTransaction
{
  auto router = [MWMRouter router];
  if (router.routeManagerTransactionId == RoutingManager::InvalidRoutePointsTransactionId())
    return;
  auto & rm = GetFramework().GetRoutingManager();
  rm.CancelRoutePointsTransaction(router.routeManagerTransactionId);
  router.routeManagerTransactionId = RoutingManager::InvalidRoutePointsTransactionId();
  rm.CancelPreviewMode();
}

+ (void)movePointAtIndex:(NSInteger)index toIndex:(NSInteger)newIndex
{
  NSAssert(index != newIndex, @"Route manager moves point to its' current position.");
  GetFramework().GetRoutingManager().MoveRoutePoint(index, newIndex);
}

+ (void)swapStartAndFinish
{
  auto const points = GetFramework().GetRoutingManager().GetRoutePoints();
  if (points.empty())
    return;
  auto & rm = GetFramework().GetRoutingManager();
  if (points.size() == 1)
  {
    RouteMarkType currentType = points[0].m_pointType;
    ASSERT(currentType != RouteMarkType::Intermediate, ("There should be no intermediate points if points count is 1"));
    RouteMarkType targetType = currentType == RouteMarkType::Start ? RouteMarkType::Finish : RouteMarkType::Start;
    rm.MoveRoutePoint(currentType, 0, targetType, 0);
  }
  else
  {
    rm.MoveRoutePoint(0, points.size() - 1);
    rm.MoveRoutePoint(points.size() - 2, 0);
  }
}

+ (void)updatePreviewMode
{
  GetFramework().GetRoutingManager().UpdatePreviewMode();
}

@end
