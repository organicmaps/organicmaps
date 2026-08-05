#include "routing/route_speed_settings.hpp"

#include "platform/settings.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <string_view>

namespace routing
{
namespace
{
// {min, max, step, default} cruising speed on flat pavement, km/h. The defaults are the speeds the
// routing profiles already predict there, so the default setting keeps the current ETA.
CruisingSpeedRange constexpr kPedestrianSpeedRange{2.0, 9.0, 0.5, 5.0};
CruisingSpeedRange constexpr kBicycleSpeedRange{8.0, 40.0, 1.0, 20.0};

std::string_view constexpr kPedestrianSpeedKey = "routing_cruising_speed_kmph_pedestrian";
std::string_view constexpr kBicycleSpeedKey = "routing_cruising_speed_kmph_bicycle";
std::string_view constexpr kWindSpeedKey = "routing_wind_speed_mps_bicycle";
std::string_view constexpr kWindDirectionKey = "routing_wind_direction_degrees_bicycle";

std::string_view GetSpeedKey(VehicleType vehicleType)
{
  return vehicleType == VehicleType::Pedestrian ? kPedestrianSpeedKey : kBicycleSpeedKey;
}
}  // namespace

bool IsRouteSpeedSupported(VehicleType vehicleType)
{
  return vehicleType == VehicleType::Pedestrian || vehicleType == VehicleType::Bicycle;
}

bool IsWindSupported(VehicleType vehicleType)
{
  return vehicleType == VehicleType::Bicycle;
}

CruisingSpeedRange GetCruisingSpeedRange(VehicleType vehicleType)
{
  CHECK(IsRouteSpeedSupported(vehicleType), (vehicleType));
  return vehicleType == VehicleType::Pedestrian ? kPedestrianSpeedRange : kBicycleSpeedRange;
}

RouteSpeedSettings LoadRouteSpeedSettings(VehicleType vehicleType)
{
  if (!IsRouteSpeedSupported(vehicleType))
    return {};

  auto const range = GetCruisingSpeedRange(vehicleType);
  RouteSpeedSettings settings{range.m_default};

  double speedKMpH = 0.0;
  if (settings::Get(GetSpeedKey(vehicleType), speedKMpH) && speedKMpH >= range.m_min && speedKMpH <= range.m_max)
    settings.m_cruisingSpeedKMpH = speedKMpH;

  int windSpeedMpS = 0;
  int windDirectionDegrees = 0;
  if (IsWindSupported(vehicleType) && settings::Get(kWindSpeedKey, windSpeedMpS) &&
      settings::Get(kWindDirectionKey, windDirectionDegrees) && windSpeedMpS > 0 && windSpeedMpS <= kMaxWindSpeedMpS &&
      windDirectionDegrees >= 0 && windDirectionDegrees < 360)
  {
    settings.m_windSpeedMpS = windSpeedMpS;
    settings.m_windDirectionDegrees = windDirectionDegrees;
  }

  return settings;
}

void SaveRouteSpeedSettings(VehicleType vehicleType, RouteSpeedSettings const & settings)
{
  if (!IsRouteSpeedSupported(vehicleType))
    return;

  auto const range = GetCruisingSpeedRange(vehicleType);
  settings::Set(GetSpeedKey(vehicleType), std::clamp(settings.m_cruisingSpeedKMpH, range.m_min, range.m_max));

  if (!IsWindSupported(vehicleType))
    return;
  settings::Set(kWindSpeedKey, std::clamp(settings.m_windSpeedMpS, 0, kMaxWindSpeedMpS));
  settings::Set(kWindDirectionKey, std::clamp(settings.m_windDirectionDegrees, 0, 359));
}
}  // namespace routing
