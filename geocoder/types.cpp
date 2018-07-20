#include "geocoder/types.hpp"

#include "base/assert.hpp"

using namespace std;

namespace geocoder
{
string ToString(Type type)
{
  switch (type)
  {
  case Type::Country: return "country"; break;
  case Type::Region: return "region"; break;
  case Type::Subregion: return "subregion"; break;
  case Type::Locality: return "locality"; break;
  case Type::Sublocality: return "sublocality"; break;
  case Type::Suburb: return "suburb"; break;
  case Type::Building: return "building"; break;
  case Type::Count: return "count"; break;
  }
  CHECK_SWITCH();
}

string DebugPrint(Type type)
{
  return ToString(type);
}
}  // namespace geocoder
