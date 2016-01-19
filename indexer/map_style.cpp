#include "indexer/map_style.hpp"

#include "base/assert.hpp"

string DebugPrint(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleLight:
    return "MapStyleLight";
  case MapStyleDark:
    return "MapStyleDark";
  case MapStyleClear:
    return "MapStyleClear";
  case MapStyleMerged:
    return "MapStyleMerged";

  case MapStyleCount:
    break;
  }
  ASSERT(false, ());
  return string();
}
