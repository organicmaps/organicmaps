#pragma once

#include "std/string.hpp"

enum class TMapOptions : uint8_t
{
  Nothing = 0x0,
  Map = 0x1,
  CarRouting = 0x2,
  MapWithCarRouting = 0x3
};

bool HasOptions(TMapOptions mask, TMapOptions options);

TMapOptions IntersectOptions(TMapOptions lhs, TMapOptions rhs);

TMapOptions SetOptions(TMapOptions mask, TMapOptions options);

TMapOptions UnsetOptions(TMapOptions mask, TMapOptions options);

TMapOptions LeastSignificantOption(TMapOptions mask);

string DebugPrint(TMapOptions options);
