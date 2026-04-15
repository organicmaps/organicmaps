#import "MWMRoutingOptions.h"

#include "routing/routing_options.hpp"
#import "MWMRouterType.h"

@interface MWMRoutingOptions ()
{
  routing::RoutingOptions _options;
  MWMRouterType _routerType;
}

@end

@implementation MWMRoutingOptions

- (instancetype)init
{
  return [self initWithRouterType:MWMRouterTypeVehicle];
}

- (instancetype)initWithRouterType:(MWMRouterType)type
{
  self = [super init];
  if (self) {
    _routerType = type;
    switch (type) {
      case MWMRouterTypeVehicle:
        _options = routing::RoutingOptions::LoadCarOptionsFromSettings();
        break;
      case MWMRouterTypePedestrian:
        _options = routing::RoutingOptions::LoadPedestrianOptionsFromSettings();
        break;
      case MWMRouterTypeBicycle:
        _options = routing::RoutingOptions::LoadBicycleOptionsFromSettings();
        break;
      default:
        _options = routing::RoutingOptions::LoadCarOptionsFromSettings();
        break;
    }
  }
  return self;
}

- (BOOL)avoidToll
{
  return _options.Has(routing::RoutingOptions::Road::Toll);
}

- (void)setAvoidToll:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Toll) enabled:avoid];
}

- (BOOL)avoidDirty
{
  return _options.Has(routing::RoutingOptions::Road::Dirty);
}

- (void)setAvoidDirty:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Dirty) enabled:avoid];
}

- (BOOL)avoidFerry
{
  return _options.Has(routing::RoutingOptions::Road::Ferry);
}

- (void)setAvoidFerry:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Ferry) enabled:avoid];
}

- (BOOL)avoidMotorway
{
  return _options.Has(routing::RoutingOptions::Road::Motorway);
}

- (void)setAvoidMotorway:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Motorway) enabled:avoid];
}

- (BOOL)hasOptions
{
  return self.avoidToll || self.avoidDirty || self.avoidFerry || self.avoidMotorway;
}

- (void)save
{
  switch (_routerType) {
    case MWMRouterTypeVehicle:
      routing::RoutingOptions::SaveCarOptionsToSettings(_options);
      break;
    case MWMRouterTypePedestrian:
      routing::RoutingOptions::SavePedestrianOptionsToSettings(_options);
      break;
    case MWMRouterTypeBicycle:
      routing::RoutingOptions::SaveBicycleOptionsToSettings(_options);
      break;
    default:
      routing::RoutingOptions::SaveCarOptionsToSettings(_options);
      break;
  }
}

- (void)setOption:(routing::RoutingOptions::Road)option enabled:(BOOL)enabled
{
  if (enabled)
    _options.Add(option);
  else
    _options.Remove(option);
}

- (BOOL)isEqual:(id)object
{
  if (![object isMemberOfClass:self.class])
    return NO;
  MWMRoutingOptions * another = (MWMRoutingOptions *)object;
  return another.avoidToll == self.avoidToll && another.avoidDirty == self.avoidDirty &&
         another.avoidFerry == self.avoidFerry && another.avoidMotorway == self.avoidMotorway;
}

@end
