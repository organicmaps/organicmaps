#include "indexer/map_style.hpp"

#include "base/assert.hpp"

#include <cstddef>

MapStyle const kDefaultMapStyle = MapStyleDefaultLight;

MapStyle MapStyleFromSettings(std::string const & str)
{
  for (size_t i = 0; i < MapStyleCount; ++i)
  {
    auto const style = static_cast<MapStyle>(i);
    // MapStyleMerged is a service style. It's unavailable for users.
    if (style != MapStyleMerged && str == MapStyleToString(style))
      return style;
  }

  return kDefaultMapStyle;
}

std::string MapStyleToString(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleDefaultDark: return "MapStyleDefaultDark";
  case MapStyleDefaultLight: return "MapStyleDefaultLight";
  case MapStyleMerged: return "MapStyleMerged";
  case MapStyleVehicleDark: return "MapStyleVehicleDark";
  case MapStyleVehicleLight: return "MapStyleVehicleLight";
  case MapStyleOutdoorsDark: return "MapStyleOutdoorsDark";
  case MapStyleOutdoorsLight: return "MapStyleOutdoorsLight";
  case MapStyleCyclingDark: return "MapStyleCyclingDark";
  case MapStyleCyclingLight: return "MapStyleCyclingLight";

  case MapStyleCount: break;
  }
  ASSERT(false, ());
  return std::string();
}

std::string DebugPrint(MapStyle mapStyle)
{
  return MapStyleToString(mapStyle);
}

bool MapStyleIsDark(MapStyle mapStyle)
{
  ASSERT(mapStyle >= MapStyleDefaultLight && mapStyle < MapStyleCount, (static_cast<int>(mapStyle)));
  for (auto const darkStyle : {MapStyleDefaultDark, MapStyleVehicleDark, MapStyleOutdoorsDark, MapStyleCyclingDark})
    if (mapStyle == darkStyle)
      return true;
  return false;
}

bool MapStyleIsVehicle(MapStyle mapStyle)
{
  return mapStyle == MapStyleVehicleLight || mapStyle == MapStyleVehicleDark;
}

MapStyleMode GetMapStyleMode(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleOutdoorsLight:
  case MapStyleOutdoorsDark: return MapStyleMode::Outdoors;
  case MapStyleCyclingLight:
  case MapStyleCyclingDark: return MapStyleMode::Cycling;
  case MapStyleDefaultLight:
  case MapStyleDefaultDark:
  case MapStyleVehicleLight:
  case MapStyleVehicleDark:
  case MapStyleMerged: return MapStyleMode::Default;
  case MapStyleCount: CHECK(false, ());
  }
  UNREACHABLE();
}

MapStyle GetMapStyleForMode(MapStyleMode mode, bool dark)
{
  switch (mode)
  {
  case MapStyleMode::Default: return dark ? MapStyleDefaultDark : MapStyleDefaultLight;
  case MapStyleMode::Outdoors: return dark ? MapStyleOutdoorsDark : MapStyleOutdoorsLight;
  case MapStyleMode::Cycling: return dark ? MapStyleCyclingDark : MapStyleCyclingLight;
  }
  UNREACHABLE();
}

std::string_view MapStyleModeToString(MapStyleMode mode)
{
  switch (mode)
  {
  case MapStyleMode::Default: return "Default";
  case MapStyleMode::Outdoors: return "Outdoors";
  case MapStyleMode::Cycling: return "Cycling";
  }
  UNREACHABLE();
}

std::optional<MapStyleMode> MapStyleModeFromSettings(std::string_view str)
{
  for (auto const mode : {MapStyleMode::Default, MapStyleMode::Outdoors, MapStyleMode::Cycling})
    if (str == MapStyleModeToString(mode))
      return mode;
  return {};
}
