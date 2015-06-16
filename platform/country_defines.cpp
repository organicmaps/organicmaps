#include "platform/country_defines.hpp"

#include "base/assert.hpp"

bool HasOptions(TMapOptions mask, TMapOptions options)
{
  return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(options)) ==
         static_cast<uint8_t>(options);
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
    case TMapOptions::ENothing:
      return "Nothing";
    case TMapOptions::EMap:
      return "MapOnly";
    case TMapOptions::ECarRouting:
      return "CarRouting";
    case TMapOptions::EMapWithCarRouting:
      return "MapWithCarRouting";
  }
}
