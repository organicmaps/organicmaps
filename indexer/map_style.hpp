#pragma once

#include "std/string.hpp"

enum MapStyle
{
  MapStyleLight = 0,
  MapStyleDark = 1,
  MapStyleClear = 2
  // Add new map style here
};

string DebugPrint(MapStyle mapStyle);
