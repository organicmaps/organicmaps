#pragma once

#include "std/string.hpp"

enum MapStyle
{
  MapStyleLight = 0, //< The first must be 0
  MapStyleDark = 1,
  MapStyleClear = 2,
  MapStyleMerged = 3,
  // Add new map style here

  // Specifies number of MapStyle enum values, must be last
  MapStyleCount
};

string DebugPrint(MapStyle mapStyle);
