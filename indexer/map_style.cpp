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

  return kDefaultMapStyle;
}

std::string MapStyleToString(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleDefaultDark:
    return "MapStyleDefaultDark";
  case MapStyleDefaultLight:
    return "MapStyleDefaultLight";
  case MapStyleMerged:
    return "MapStyleMerged";
  case MapStyleVehicleDark:
    return "MapStyleVehicleDark";
  case MapStyleVehicleLight:
    return "MapStyleVehicleLight";
  case MapStyleOutdoorsDark:
    return "MapStyleOutdoorsDark";
  case MapStyleOutdoorsLight:
    return "MapStyleOutdoorsLight";

  case MapStyleCount:
    break;
  }
  ASSERT(false, ());
  return std::string();
}

std::string DebugPrint(MapStyle mapStyle)
{
  return MapStyleToString(mapStyle);
}
