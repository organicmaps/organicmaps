#pragma once

#include "std/string.hpp"

namespace search
{
enum class Mode
{
  Everywhere,
  Viewport,
  Downloader,
  Bookmarks,
  Count
};

string DebugPrint(Mode mode);
}  // namespace search
