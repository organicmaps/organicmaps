#pragma once

#include "std/string.hpp"

namespace search
{
enum class Mode
{
  Viewport,
  Everywhere,
  World
};

string DebugPrint(Mode mode);
}  // namespace search
