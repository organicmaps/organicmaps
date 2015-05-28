#include "platform/country_defines.hpp"

#include "base/assert.hpp"

string DebugPrint(TMapOptions options)
{
  switch (options)
  {
    case TMapOptions::ENothing:
      return "Nothing";
    case TMapOptions::EMapOnly:
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
