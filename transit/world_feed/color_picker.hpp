#pragma once

#include <string>
#include <unordered_map>

namespace transit
{
class ColorPicker
{
public:
  ColorPicker();
  std::string GetNearestColor(std::string const & rgb);

private:
  std::unordered_map<std::string, std::string> m_colorsToNames;
};
}  // namespace transit
