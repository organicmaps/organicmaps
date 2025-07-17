#pragma once

#include <string>

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

std::string DebugPrint(Mode mode);
}  // namespace search
