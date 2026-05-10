#include "platform/country_defines.hpp"

#include "base/assert.hpp"

std::string_view DebugPrint(MapFileType type)
{
  switch (type)
  {
    using enum MapFileType;
  case Map: return "Map";
  case Diff: return "Diff";
  case Count: return "Count";
  }
  UNREACHABLE();
}
