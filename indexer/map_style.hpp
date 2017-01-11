#pragma once

#include <string>

enum MapStyle
{
  MapStyleClear = 0,
  MapStyleDark = 1,
  MapStyleMerged = 2,
  // Add new map style here

  // Specifies number of MapStyle enum values, must be last
  MapStyleCount
};

extern MapStyle const kDefaultMapStyle;

extern MapStyle MapStyleFromSettings(std::string const & str);
extern std::string MapStyleToString(MapStyle mapStyle);
extern std::string DebugPrint(MapStyle mapStyle);
