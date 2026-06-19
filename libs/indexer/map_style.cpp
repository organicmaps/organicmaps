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
  case MapStyleDefaultDark: return "MapStyleDefaultDark";
  case MapStyleDefaultLight: return "MapStyleDefaultLight";
  case MapStyleMerged: return "MapStyleMerged";
  case MapStyleVehicleDark: return "MapStyleVehicleDark";
  case MapStyleVehicleLight: return "MapStyleVehicleLight";
  case MapStyleOutdoorsDark: return "MapStyleOutdoorsDark";
  case MapStyleOutdoorsLight: return "MapStyleOutdoorsLight";

  case MapStyleCount: break;
  }
  ASSERT(false, ());
  return std::string();
}

std::string DebugPrint(MapStyle mapStyle)
{
  return MapStyleToString(mapStyle);
}

std::string DebugPrint(MapStyleFamily family)
{
  switch (family)
  {
  case MapStyleFamily::Default: return "Default";
  case MapStyleFamily::Vehicle: return "Vehicle";
  case MapStyleFamily::Outdoors: return "Outdoors";
  }
  return "Unknown MapStyleFamily";
}

bool MapStyleIsDark(MapStyle mapStyle)
{
  for (auto const darkStyle : {MapStyleDefaultDark, MapStyleVehicleDark, MapStyleOutdoorsDark})
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
  default: CHECK(false, ()); return MapStyleDefaultLight;
  }
}

MapStyle GetMapStyleForFamily(MapStyleFamily family, bool dark)
{
  switch (family)
  {
  case MapStyleFamily::Default: return dark ? MapStyleDefaultDark : MapStyleDefaultLight;
  case MapStyleFamily::Vehicle: return dark ? MapStyleVehicleDark : MapStyleVehicleLight;
  case MapStyleFamily::Outdoors: return dark ? MapStyleOutdoorsDark : MapStyleOutdoorsLight;
  }
  CHECK(false, ("Unhandled MapStyleFamily", static_cast<int>(family)));
  return dark ? MapStyleDefaultDark : MapStyleDefaultLight;
}

bool IsOutdoorsStyle(MapStyle mapStyle)
{
  return mapStyle == MapStyleOutdoorsLight || mapStyle == MapStyleOutdoorsDark;
}

MapStyleFamily SelectMapStyleFamily(bool vehicleFollowing, bool outdoorsEnabled)
{
  // Priority, highest first. A new higher-priority family (e.g. Satellite) prepends its own branch.
  if (vehicleFollowing)
    return MapStyleFamily::Vehicle;
  if (outdoorsEnabled)
    return MapStyleFamily::Outdoors;
  return MapStyleFamily::Default;
}

StartupMapStyle NormalizeStartupMapStyle(MapStyle persisted, std::optional<bool> persistedOutdoorsEnabled)
{
  // The stored flag is authoritative; only a legacy Outdoors-only persistence (flag absent) derives it
  // from the style. Persist only that derived value, so an explicit stored value is never overwritten.
  bool const outdoorsEnabled = persistedOutdoorsEnabled.value_or(IsOutdoorsStyle(persisted));
  bool const persistFlag = !persistedOutdoorsEnabled.has_value() && outdoorsEnabled;
  // No active route at launch, so no Vehicle family: pick the base family at the persisted darkness.
  auto const family = SelectMapStyleFamily(false /* vehicleFollowing */, outdoorsEnabled);
  return {GetMapStyleForFamily(family, MapStyleIsDark(persisted)), outdoorsEnabled, persistFlag};
}
