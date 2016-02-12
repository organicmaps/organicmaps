#include "search/mode.hpp"

namespace search
{
string DebugPrint(Mode mode)
{
  switch (mode)
  {
  case Mode::Viewport: return "Viewport";
  case Mode::Everywhere: return "Everywhere";
  case Mode::World: return "World";
  }
  return "Unknown";
}
}  // namespace search
