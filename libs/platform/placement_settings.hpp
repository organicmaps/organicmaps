#pragma once

#include <string>

namespace settings
{
std::string_view constexpr kBookmarksTextPlacement = "BookmarksTextPlacement";

enum class Placement
{
  None = 0,
  Right,
  Bottom,
  Count
};

auto constexpr kDefaultBookmarksTextPlacement{Placement::None};

inline bool FromString(std::string const & str, Placement & v)
{
  if (str == "None")
    v = Placement::None;
  else if (str == "Right")
    v = Placement::Right;
  else if (str == "Bottom")
    v = Placement::Bottom;
  else
    return false;
  return true;
}

inline std::string ToString(Placement const & v)
{
  switch (v)
  {
  case Placement::Right: return "Right";
  case Placement::Bottom: return "Bottom";
  default: return "None";
  }
}

}  // namespace settings
