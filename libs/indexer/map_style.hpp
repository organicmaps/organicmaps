#pragma once

#include <string>

enum MapStyle
{
  MapStyleDefaultLight = 0,
  MapStyleDefaultDark = 1,
  MapStyleMerged = 2,
  MapStyleVehicleLight = 3,
  MapStyleVehicleDark = 4,
  MapStyleOutdoorsLight = 5,
  MapStyleOutdoorsDark = 6,
  // Add new map style here

  // Specifies number of MapStyle enum values, must be last
  MapStyleCount
};

// A map style "family": the variant selected by the current usage mode, orthogonal to light/dark.
// Extend this enum to introduce a new family.
enum class MapStyleFamily
{
  Default,
  Vehicle,
  Outdoors,
};

extern MapStyle const kDefaultMapStyle;

extern MapStyle MapStyleFromSettings(std::string const & str);
extern std::string MapStyleToString(MapStyle mapStyle);
extern std::string DebugPrint(MapStyle mapStyle);
extern bool MapStyleIsDark(MapStyle mapStyle);
extern MapStyle GetDarkMapStyleVariant(MapStyle mapStyle);
extern MapStyle GetLightMapStyleVariant(MapStyle mapStyle);
// Concrete map style for the given family at the given darkness.
extern MapStyle GetMapStyleForFamily(MapStyleFamily family, bool dark);
