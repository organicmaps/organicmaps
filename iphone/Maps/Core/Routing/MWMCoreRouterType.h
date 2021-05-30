#import "MWMRouterType.h"

#include "routing/router.hpp"

static inline routing::RouterType coreRouterType(MWMRouterType type)
{
  switch (type)
  {
  case MWMRouterTypeVehicle: return routing::RouterType::Vehicle;
  case MWMRouterTypePedestrian: return routing::RouterType::Pedestrian;
  case MWMRouterTypePublicTransport: return routing::RouterType::Transit;
  case MWMRouterTypeBicycle: return routing::RouterType::Bicycle;
  default:
    ASSERT(false, ("Invalid routing type"));
    return routing::RouterType::Vehicle;
  }
}

static inline MWMRouterType routerType(routing::RouterType type)
{
  switch (type)
  {
  case routing::RouterType::Vehicle: return MWMRouterTypeVehicle;
  case routing::RouterType::Transit: return MWMRouterTypePublicTransport;
  case routing::RouterType::Pedestrian: return MWMRouterTypePedestrian;
  case routing::RouterType::Bicycle: return MWMRouterTypeBicycle;
  default:
    ASSERT(false, ("Invalid routing type"));
    return MWMRouterTypeVehicle;
  }
}
