#include "indexer/map_style.hpp"

#include "base/assert.hpp"

MapStyle const kDefaultMapStyle = MapStyleClear;

MapStyle MapStyleFromSettings(std::string const & str)
{
  // MapStyleMerged is service style. It's unavailable for users.
  if (str == "MapStyleClear")
    return MapStyleClear;
  else if (str == "MapStyleDark")
    return MapStyleDark;
  else if (str == "MapStyleVehicleClear")
    return MapStyleVehicleClear;
  else if (str == "MapStyleVehicleDark")
    return MapStyleVehicleDark;

  return kDefaultMapStyle;
}

std::string MapStyleToString(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleDark:
    return "MapStyleDark";
  case MapStyleClear:
    return "MapStyleClear";
  case MapStyleMerged:
    return "MapStyleMerged";
  case MapStyleVehicleDark:
    return "MapStyleVehicleDark";
  case MapStyleVehicleClear:
    return "MapStyleVehicleClear";

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
