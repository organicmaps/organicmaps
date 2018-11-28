#include "geocoder/types.hpp"

#include "base/assert.hpp"

using namespace std;

namespace geocoder
{
string ToString(Type type)
{
  switch (type)
  {
  case Type::Country: return "country";
  case Type::Region: return "region";
  case Type::Subregion: return "subregion";
  case Type::Locality: return "locality";
  case Type::Suburb: return "suburb";
  case Type::Sublocality: return "sublocality";
  case Type::Street: return "street";
  case Type::Building: return "building";
  case Type::Count: return "count";
  }
  UNREACHABLE();
}

string DebugPrint(Type type)
{
  return ToString(type);
}
}  // namespace geocoder
