#pragma once

#include "std/string.hpp"

enum class TMapOptions : uint8_t
{
  ENothing = 0x0,
  EMap = 0x1,
  ECarRouting = 0x2,
  EMapWithCarRouting = 0x3
};

bool HasOptions(TMapOptions options, TMapOptions bits);

TMapOptions SetOptions(TMapOptions options, TMapOptions bits);

string DebugPrint(TMapOptions options);
