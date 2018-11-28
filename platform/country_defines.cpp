#include "platform/country_defines.hpp"

#include "base/assert.hpp"

bool HasOptions(MapOptions mask, MapOptions options)
{
  return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(options)) ==
         static_cast<uint8_t>(options);
}

MapOptions SetOptions(MapOptions mask, MapOptions options)
{
  return static_cast<MapOptions>(static_cast<uint8_t>(mask) | static_cast<uint8_t>(options));
}

MapOptions UnsetOptions(MapOptions mask, MapOptions options)
{
  return static_cast<MapOptions>(static_cast<uint8_t>(mask) & ~static_cast<uint8_t>(options));
}

MapOptions LeastSignificantOption(MapOptions mask)
{
  return static_cast<MapOptions>(static_cast<uint8_t>(mask) & -static_cast<uint8_t>(mask));
}

string DebugPrint(MapOptions options)
{
  switch (options)
  {
    case MapOptions::Nothing:
      return "Nothing";
    case MapOptions::Map:
      return "MapOnly";
    case MapOptions::CarRouting:
      return "CarRouting";
    case MapOptions::MapWithCarRouting:
      return "MapWithCarRouting";
    case MapOptions::Diff:
      return "Diff";
  }
  UNREACHABLE();
}
