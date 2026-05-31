#import "MWMRoutingOptions.h"

#include <CoreApi/Framework.h>

#include "routing/routing_options.hpp"

@interface MWMRoutingOptions ()
{
  routing::RoutingOptions _options;
  routing::RoutingOptions _bicycleOptions;
  BOOL _publicBicycle;
}

@end

@implementation MWMRoutingOptions

- (routing::RoutingOptions const &)activeOptions
{
  return GetFramework().GetRoutingManager().GetRouter() == routing::RouterType::Bicycle ? _bicycleOptions : _options;
}

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    _options = routing::RoutingOptions::LoadCarOptionsFromSettings();
    _bicycleOptions = routing::RoutingOptions::LoadBicycleOptionsFromSettings();
    _publicBicycle = routing::RoutingOptions::IsPublicBicycleEnabled();
  }

  return self;
}

- (BOOL)avoidToll
{
  return [self activeOptions].Has(routing::RoutingOptions::Road::Toll);
}

- (void)setAvoidToll:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Toll) enabled:avoid];
}

- (BOOL)avoidDirty
{
  return [self activeOptions].Has(routing::RoutingOptions::Road::Dirty);
}

- (void)setAvoidDirty:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Dirty) enabled:avoid];
}

- (BOOL)avoidFerry
{
  return [self activeOptions].Has(routing::RoutingOptions::Road::Ferry);
}

- (void)setAvoidFerry:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Ferry) enabled:avoid];
}

- (BOOL)avoidMotorway
{
  return [self activeOptions].Has(routing::RoutingOptions::Road::Motorway);
}

- (void)setAvoidMotorway:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Motorway) enabled:avoid];
}

- (BOOL)publicBicycle
{
  return _publicBicycle;
}

- (void)setPublicBicycle:(BOOL)enabled
{
  _publicBicycle = enabled;
}

- (void)save
{
  routing::RoutingOptions::SaveCarOptionsToSettings(_options);
  routing::RoutingOptions::SaveBicycleOptionsToSettings(_bicycleOptions);
  routing::RoutingOptions::SetPublicBicycleEnabled(_publicBicycle);
}

- (void)setOption:(routing::RoutingOptions::Road)option enabled:(BOOL)enabled
{
  routing::RoutingOptions & options =
      GetFramework().GetRoutingManager().GetRouter() == routing::RouterType::Bicycle ? _bicycleOptions : _options;
  if (enabled)
    options.Add(option);
  else
    options.Remove(option);
}

- (BOOL)isEqual:(id)object
{
  if (![object isMemberOfClass:self.class])
    return NO;
  MWMRoutingOptions * another = (MWMRoutingOptions *)object;
  return another.avoidToll == self.avoidToll && another.avoidDirty == self.avoidDirty &&
         another.avoidFerry == self.avoidFerry && another.avoidMotorway == self.avoidMotorway &&
         another.publicBicycle == self.publicBicycle;
}

@end
