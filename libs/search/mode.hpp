#pragma once

#include <string_view>

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

std::string_view DebugPrint(Mode mode);
}  // namespace search
