#include "platform/country_defines.hpp"

#include "base/assert.hpp"

bool HasOptions(TMapOptions mask, TMapOptions options)
{
  return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(options)) ==
         static_cast<uint8_t>(options);
}

TMapOptions IntersectOptions(TMapOptions lhs, TMapOptions rhs)
{
  return static_cast<TMapOptions>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

TMapOptions SetOptions(TMapOptions mask, TMapOptions options)
{
  return static_cast<TMapOptions>(static_cast<uint8_t>(mask) | static_cast<uint8_t>(options));
}

TMapOptions UnsetOptions(TMapOptions mask, TMapOptions options)
{
  return static_cast<TMapOptions>(static_cast<uint8_t>(mask) & ~static_cast<uint8_t>(options));
}

TMapOptions LeastSignificantOption(TMapOptions mask)
{
  return static_cast<TMapOptions>(static_cast<uint8_t>(mask) & -static_cast<uint8_t>(mask));
}

string DebugPrint(TMapOptions options)
{
  switch (options)
  {
    case TMapOptions::Nothing:
      return "Nothing";
    case TMapOptions::Map:
      return "MapOnly";
    case TMapOptions::CarRouting:
      return "CarRouting";
    case TMapOptions::MapWithCarRouting:
      return "MapWithCarRouting";
  }
}
