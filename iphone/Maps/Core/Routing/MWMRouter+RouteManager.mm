#import "MWMRouter.h"

#include "Framework.h"

@interface MWMRouter ()

@property(nonatomic) uint32_t routeManagerTransactionId;

+ (MWMRouter *)router;

@end

@implementation MWMRouter (RouteManager)

+ (void)openRouteManagerTransaction
{
  auto router = [MWMRouter router];
  router.routeManagerTransactionId =
      GetFramework().GetRoutingManager().OpenRoutePointsTransaction();
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
  GetFramework().GetRoutingManager().CancelRoutePointsTransaction(router.routeManagerTransactionId);
  router.routeManagerTransactionId = RoutingManager::InvalidRoutePointsTransactionId();
}

+ (void)movePointAtIndex:(NSInteger)index toIndex:(NSInteger)newIndex
{
  NSAssert(index != newIndex, @"Route manager moves point to its' current position.");
  GetFramework().GetRoutingManager().MoveRoutePoint(index, newIndex);
}

@end
