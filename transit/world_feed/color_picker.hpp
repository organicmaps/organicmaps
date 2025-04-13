#pragma once

#include "drape_frontend/color_constants.hpp"

#include <map>
#include <string>
#include <unordered_map>

namespace transit
{
class ColorPicker
{
public:
  ColorPicker();
  // Picks the closest match for the |rgb| color from our transit palette.
  std::string GetNearestColor(std::string const & rgb);

private:
  std::unordered_map<std::string, std::string> m_colorsToNames;
  std::map<std::string, dp::Color> m_drapeClearColors;
};
}  // namespace transit
