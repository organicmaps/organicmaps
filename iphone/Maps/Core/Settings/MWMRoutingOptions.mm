#import "MWMRoutingOptions.h"

#import "MWMCoreRouterType.h"

#include "routing/routing_options.hpp"

static_assert(MWMRoutingAvoidanceToll == static_cast<NSUInteger>(routing::RoutingOptions::Toll));
static_assert(MWMRoutingAvoidanceMotorway == static_cast<NSUInteger>(routing::RoutingOptions::Motorway));
static_assert(MWMRoutingAvoidanceFerry == static_cast<NSUInteger>(routing::RoutingOptions::Ferry));
static_assert(MWMRoutingAvoidanceDirty == static_cast<NSUInteger>(routing::RoutingOptions::Dirty));

@interface MWMRoutingOptions ()
{
  routing::RoutingOptions _options;
}

@end

@implementation MWMRoutingOptions

- (instancetype)initWithRouterType:(MWMRouterType)routerType
{
  self = [super init];
  if (self)
  {
    _routerType = routerType;
    auto const vehicle = routing::RoutingOptions::GetVehicleType(coreRouterType(routerType));
    if (vehicle)
    {
      _options = routing::RoutingOptions::LoadFromSettings(*vehicle);
      _supportedOptions = static_cast<MWMRoutingAvoidance>(routing::RoutingOptions::GetSupportedOptions(*vehicle));
      _supportsSavedTracks = routing::RoutingOptions::SupportsSavedTracks(*vehicle);
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

- (BOOL)preferSavedTracks
{
  return _options.PreferSavedTracks();
}

- (void)setPreferSavedTracks:(BOOL)enabled
{
  _options.SetPreferSavedTracks(enabled && self.supportsSavedTracks);
}

- (BOOL)routeOptimizationEnabled
{
  return routing::RoutingOptions::LoadRouteOptimizationFromSettings();
}

- (void)setRouteOptimizationEnabled:(BOOL)enabled
{
  routing::RoutingOptions::SaveRouteOptimizationToSettings(enabled);
}

- (void)save
{
  auto const vehicle = routing::RoutingOptions::GetVehicleType(coreRouterType(self.routerType));
  if (vehicle)
    routing::RoutingOptions::SaveToSettings(*vehicle, _options);
}

- (void)setOption:(routing::RoutingOptions::Road)option enabled:(BOOL)enabled
{
  if (enabled && (self.supportedOptions & option))
    _options.Add(option);
  else
    _options.Remove(option);
}

- (BOOL)isEqual:(id)object
{
  if (![object isMemberOfClass:self.class])
    return NO;
  MWMRoutingOptions * another = (MWMRoutingOptions *)object;
  // Optimization only changes stop order and does not require a route rebuild here.
  return another.routerType == self.routerType && another->_options.GetOptions() == _options.GetOptions() &&
         another.preferSavedTracks == self.preferSavedTracks;
}

- (NSUInteger)hash
{
  return (static_cast<NSUInteger>(self.routerType) << 9) | (self.preferSavedTracks << 8) | _options.GetOptions();
}

@end
