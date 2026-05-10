#include "search/mode.hpp"

namespace search
{
std::string_view DebugPrint(Mode mode)
{
  switch (mode)
  {
    using enum Mode;
  case Everywhere: return "Everywhere";
  case Viewport: return "Viewport";
  case Downloader: return "Downloader";
  case Bookmarks: return "Bookmarks";
  case Count: return "Count";
  }
  return "Unknown";
}
}  // namespace search
