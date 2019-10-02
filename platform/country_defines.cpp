#include "platform/country_defines.hpp"

#include "base/assert.hpp"

std::string DebugPrint(MapOptions options)
{
  switch (options)
  {
  case MapOptions::Map: return "MapOnly";
  case MapOptions::Diff: return "Diff";
  }
  UNREACHABLE();
}
