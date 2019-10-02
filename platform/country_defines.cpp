#include "platform/country_defines.hpp"

#include "base/assert.hpp"

std::string DebugPrint(MapFileType type)
{
  switch (type)
  {
  case MapFileType::Map: return "MapOnly";
  case MapFileType::Diff: return "Diff";
  }
  UNREACHABLE();
}
