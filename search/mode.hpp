#pragma once

#include "std/string.hpp"

namespace search
{
enum class Mode
{
  Everywhere,
  Viewport,
  Downloader,
  Count
};

string DebugPrint(Mode mode);
}  // namespace search
