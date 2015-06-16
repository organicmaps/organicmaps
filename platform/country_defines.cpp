#include "platform/country_defines.hpp"

#include "base/assert.hpp"

bool HasOptions(TMapOptions options, TMapOptions bits)
{
  return (static_cast<uint8_t>(options) & static_cast<uint8_t>(bits)) == static_cast<uint8_t>(bits);
}

TMapOptions SetOptions(TMapOptions options, TMapOptions bits)
{
  return static_cast<TMapOptions>(static_cast<uint8_t>(options) | static_cast<uint8_t>(bits));
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
    default:
      ASSERT(false, ("Unknown TMapOptions (", static_cast<uint8_t>(options), ")"));
      return string();
  }
}
