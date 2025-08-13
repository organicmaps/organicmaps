#include "color_parser.hpp"

#include "coding/hex.hpp"

#include "base/string_utils.hpp"

namespace kml
{

struct RGBColor
{
  uint8_t r, g, b;
};

std::optional<uint32_t> ParseHexColor(std::string_view c)
{
  if (c.empty())
    return {};

  if (c.front() == '#')
    c.remove_prefix(1);
  if (c.size() != 6 && c.size() != 8)
    return {};

  auto const colorBytes = FromHex(c);
  switch (colorBytes.size())
  {
  case 3: return ToRGBA(colorBytes[0], colorBytes[1], colorBytes[2]);
  case 4: return ToRGBA(colorBytes[1], colorBytes[2], colorBytes[3], colorBytes[0]);
  default: return {};
  }
}

// Garmin extensions spec: https://www8.garmin.com/xmlschemas/GpxExtensionsv3.xsd
// Color mapping: https://help.locusmap.eu/topic/extend-garmin-gpx-compatibilty
std::optional<uint32_t> ParseGarminColor(std::string_view c)
{
  /// @todo Unify with RGBColor instead of string.
  static std::pair<std::string_view, std::string_view> arrColors[] = {
      {"Black", "000000"},    {"DarkRed", "8b0000"},     {"DarkGreen", "006400"}, {"DarkYellow", "b5b820"},
      {"DarkBlue", "00008b"}, {"DarkMagenta", "8b008b"}, {"DarkCyan", "008b8b"},  {"LightGray", "cccccc"},
      {"DarkGray", "444444"}, {"Red", "ff0000"},         {"Green", "00ff00"},     {"Yellow", "ffff00"},
      {"Blue", "0000ff"},     {"Magenta", "ff00ff"},     {"Cyan", "00ffff"},      {"White", "ffffff"}};

  for (auto const & e : arrColors)
    if (c == e.first)
      return ParseHexColor(e.second);

  return {};
}

std::optional<uint32_t> ParseOSMColor(std::string_view c)
{
  static std::pair<std::string_view, RGBColor> arrColors[] = {
      {"black", {0, 0, 0}},
      {"white", {255, 255, 255}},
      {"red", {255, 0, 0}},
      {"green", {0, 128, 0}},
      {"blue", {0, 0, 255}},
      {"yellow", {255, 255, 0}},
      {"orange", {255, 165, 0}},
      {"gray", {128, 128, 128}},
      {"grey", {128, 128, 128}},  // British spelling
      {"brown", {165, 42, 42}},
      {"pink", {255, 192, 203}},
      {"purple", {128, 0, 128}},
      {"cyan", {0, 255, 255}},
      {"magenta", {255, 0, 255}},

      {"maroon", {128, 0, 0}},
      {"olive", {128, 128, 0}},
      {"teal", {0, 128, 128}},
      {"navy", {0, 0, 128}},
      {"silver", {192, 192, 192}},
      {"lime", {0, 255, 0}},
      {"aqua", {0, 255, 255}},     // cyan
      {"fuchsia", {255, 0, 255}},  // magenta

      // From top taginfo for "colour" and CSS standart values.
      {"darkgreen", {0, 100, 0}},
      {"beige", {245, 245, 220}},
      {"dimgray", {105, 105, 105}},
      {"lightgrey", {211, 211, 211}},  // British spelling
      {"lightgray", {211, 211, 211}},
      {"tan", {210, 180, 140}},
      {"gold", {255, 215, 0}},

      {"red;white", {255, 127, 127}},
      {"red and white", {255, 127, 127}},
      {"red-white", {255, 127, 127}},
  };

  if (!c.empty())
  {
    if (c[0] == '#')
    {
      using strings::to_uint;
      if (c.size() == 7)  // #rrggbb
      {
        uint8_t r, g, b;
        if (to_uint(c.substr(1, 2), r, 16) && to_uint(c.substr(3, 2), g, 16) && to_uint(c.substr(5, 2), b, 16))
          return ToRGBA(r, g, b);
      }
      else if (c.size() == 4)  // #rgb shorthand
      {
        uint8_t r, g, b;
        if (to_uint(c.substr(1, 1), r, 16) && to_uint(c.substr(2, 1), g, 16) && to_uint(c.substr(3, 1), b, 16))
          return ToRGBA(uint8_t(r * 17), uint8_t(g * 17), uint8_t(b * 17));
      }
    }
    else
    {
      for (auto const & e : arrColors)
        if (c == e.first)
          return ToRGBA(e.second.r, e.second.g, e.second.b);
    }
  }

  return {};
}

}  // namespace kml
