#include "search/mode.hpp"

namespace search
{
std::string DebugPrint(Mode mode)
{
  switch (mode)
  {
  case Mode::Everywhere: return "Everywhere";
  case Mode::Viewport: return "Viewport";
  case Mode::Downloader: return "Downloader";
  case Mode::Bookmarks: return "Bookmarks";
  case Mode::Count: return "Count";
  }
  return "Unknown";
}
}  // namespace search
