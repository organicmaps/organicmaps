#pragma once

#include <cstdint>
#include <string>
#include <utility>

// Note: do not forget to change kMapOptionsCount
// value when count of elements is changed.
enum class MapOptions : uint8_t
{
  Nothing = 0x0,
  Map = 0x1,
  CarRouting = 0x2,
  MapWithCarRouting = 0x3,
  Diff = 0x4
};

uint8_t constexpr kMapOptionsCount = 5;

using MwmCounter = uint32_t;
using MwmSize = uint64_t;
using LocalAndRemoteSize = std::pair<MwmSize, MwmSize>;

bool HasOptions(MapOptions mask, MapOptions options);

MapOptions SetOptions(MapOptions mask, MapOptions options);

MapOptions UnsetOptions(MapOptions mask, MapOptions options);

MapOptions LeastSignificantOption(MapOptions mask);

std::string DebugPrint(MapOptions options);
