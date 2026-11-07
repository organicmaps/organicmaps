#pragma once

#include <cstdint>
#include <string>
#include <utility>

// Note: new values must be added before MapFileType::Count.
enum class MapFileType : uint8_t
{
  Map,
  Diff,
  // A terrain (.twm) block, keyed by the block name (see storage terrain downloading);
  // never registered as a LocalCountryFile - the terrain registry is the TwmSet.
  Terrain,

  Count
};

using MwmCounter = uint32_t;
using MwmSize = uint64_t;
using LocalAndRemoteSize = std::pair<MwmSize, MwmSize>;

std::string DebugPrint(MapFileType type);
