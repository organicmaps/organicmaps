#pragma once

#include "std/string.hpp"
#include "3party/enum_flags.hpp"

ENUM_FLAGS(TMapOptions)
enum class TMapOptions
{
  ENothing = 0x0,
  EMapOnly = 0x1,
  ECarRouting = 0x2,
  EMapWithCarRouting = 0x3
};

string DebugPrint(TMapOptions options);
