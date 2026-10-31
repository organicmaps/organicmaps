#include "indexer/map_style.hpp"

#include "base/assert.hpp"

MapStyle const kDefaultMapStyle = MapStyleDefaultLight;

MapStyle MapStyleFromSettings(std::string const & str)
{
  // MapStyleMerged is service style. It's unavailable for users.
  if (str == "MapStyleDefaultLight")
    return MapStyleDefaultLight;
  else if (str == "MapStyleDefaultDark")
    return MapStyleDefaultDark;
  else if (str == "MapStyleVehicleLight")
    return MapStyleVehicleLight;
  else if (str == "MapStyleVehicleDark")
    return MapStyleVehicleDark;
  else if (str == "MapStyleOutdoorsLight")
    return MapStyleOutdoorsLight;
  else if (str == "MapStyleOutdoorsDark")
    return MapStyleOutdoorsDark;
  else if (str == "MapStyleExplorerLight")
    return MapStyleExplorerLight;
  else if (str == "MapStyleExplorerDark")
    return MapStyleExplorerDark;

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
  case MapStyleExplorerDark: return "MapStyleExplorerDark";
  case MapStyleExplorerLight: return "MapStyleExplorerLight";

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
  for (auto const darkStyle : {MapStyleDefaultDark, MapStyleVehicleDark, MapStyleOutdoorsDark, MapStyleExplorerDark})
    if (mapStyle == darkStyle)
      return true;
  return false;
}

MapStyle GetDarkMapStyleVariant(MapStyle mapStyle)
{
  if (MapStyleIsDark(mapStyle) || mapStyle == MapStyleMerged)
    return mapStyle;

  switch (mapStyle)
  {
  case MapStyleDefaultLight: return MapStyleDefaultDark;
  case MapStyleVehicleLight: return MapStyleVehicleDark;
  case MapStyleOutdoorsLight: return MapStyleOutdoorsDark;
  case MapStyleExplorerLight: return MapStyleExplorerDark;
  default: CHECK(false, ()); return MapStyleDefaultDark;
  }
}

MapStyle GetLightMapStyleVariant(MapStyle mapStyle)
{
  if (!MapStyleIsDark(mapStyle))
    return mapStyle;

  switch (mapStyle)
  {
  case MapStyleDefaultDark: return MapStyleDefaultLight;
  case MapStyleVehicleDark: return MapStyleVehicleLight;
  case MapStyleOutdoorsDark: return MapStyleOutdoorsLight;
  case MapStyleExplorerDark: return MapStyleExplorerLight;
  default: CHECK(false, ()); return MapStyleDefaultLight;
  }
}
