#import "MWMRouteSpeedSettings.h"

#include <CoreApi/Framework.h>

#include "routing/route_speed_settings.hpp"

NSInteger constexpr kDefaultWindSpeedMpS = 3;

@interface MWMRouteSpeedSettings ()

- (instancetype)initWithVehicleType:(routing::VehicleType)vehicleType;

@end

@implementation MWMRouteSpeedSettings

+ (nullable instancetype)current
{
  auto const vehicleType = GetFramework().GetRoutingManager().GetRouterVehicleType();
  if (!routing::IsRouteSpeedSupported(vehicleType))
    return nil;
  return [[MWMRouteSpeedSettings alloc] initWithVehicleType:vehicleType];
}

- (instancetype)initWithVehicleType:(routing::VehicleType)vehicleType
{
  self = [super init];
  if (self)
  {
    auto const settings = routing::LoadRouteSpeedSettings(vehicleType);
    auto const range = routing::GetCruisingSpeedRange(vehicleType);
    _cruisingSpeedKMpH = settings.m_cruisingSpeedKMpH;
    _windSpeedMpS = settings.m_windSpeedMpS;
    _windDirectionDegrees = settings.m_windDirectionDegrees;
    _minimumSpeedKMpH = range.m_min;
    _maximumSpeedKMpH = range.m_max;
    _speedStepKMpH = range.m_step;
    _defaultSpeedKMpH = range.m_default;
    _maximumWindSpeedMpS = routing::IsWindSupported(vehicleType) ? routing::kMaxWindSpeedMpS : 0;
  }
  return self;
}

- (BOOL)windSupported
{
  return self.maximumWindSpeedMpS > 0;
}

- (NSInteger)changedCount
{
  return (self.cruisingSpeedKMpH == self.defaultSpeedKMpH ? 0 : 1) + (self.windSpeedMpS > 0 ? 1 : 0);
}

+ (NSInteger)defaultWindSpeedMpS
{
  return kDefaultWindSpeedMpS;
}

+ (NSInteger)windDirectionStepDegrees
{
  return routing::kWindDirectionStepDegrees;
}

- (void)save
{
  GetFramework().GetRoutingManager().SetRouteSpeedSettings(
      {self.cruisingSpeedKMpH, static_cast<int>(self.windSpeedMpS), static_cast<int>(self.windDirectionDegrees)});
}

@end
