#pragma once

#include <string>

struct MapStyleConfig
{
  struct ThemeConfig
  {
    std::string drulesPath;
    std::string symbolsPath;
  };

  std::string name;
  ThemeConfig light;
  ThemeConfig dark;
  std::string defaultResourcesPath;
  std::string colorsPath;
  std::string transitColorsPath;
  std::string patternsPath;

  std::string styleDir;
};
