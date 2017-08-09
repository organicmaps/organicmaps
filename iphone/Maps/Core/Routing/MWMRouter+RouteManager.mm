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
  NSMutableArray<MWMRoutePoint *> * points = [[MWMRouter points] mutableCopy];

  auto removeIndex = index;
  auto insertIndex = newIndex;

  if (index < newIndex)
    insertIndex += 1;
  else
    removeIndex += 1;

  [points insertObject:points[index] atIndex:insertIndex];
  [points removeObjectAtIndex:removeIndex];

  [MWMRouter removePoints];

  [points enumerateObjectsUsingBlock:^(MWMRoutePoint * point, NSUInteger idx, BOOL * stop) {
    if (idx == 0)
    {
      point.type = MWMRoutePointTypeStart;
      point.intermediateIndex = 0;
    }
    else if (idx == points.count - 1)
    {
      point.type = MWMRoutePointTypeFinish;
      point.intermediateIndex = 0;
    }
    else
    {
      point.type = MWMRoutePointTypeIntermediate;
      point.intermediateIndex = idx - 1;
    }
    [MWMRouter addPoint:point];
  }];
}

@end
