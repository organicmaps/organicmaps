#pragma once

#include <cstdint>
#include <string>
#include <utility>

// Note: do not forget to change kMapOptionsCount
// value when count of elements is changed.
enum class MapOptions : uint8_t
{
  Map,
  Diff
};

uint8_t constexpr kMapOptionsCount = 2;

using MwmCounter = uint32_t;
using MwmSize = uint64_t;
using LocalAndRemoteSize = std::pair<MwmSize, MwmSize>;

std::string DebugPrint(MapOptions options);
