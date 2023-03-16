#pragma once

#include <cstdint>
#include <string>
#include <utility>

// Note: new values must be added before MapFileType::Count.
enum class MapFileType : uint8_t
{
  Map,
  Diff,

  Count
};

using MwmCounter = uint32_t;
using MwmSize = uint64_t;
using LocalAndRemoteSize = std::pair<MwmSize, MwmSize>;

std::string DebugPrint(MapFileType type);
