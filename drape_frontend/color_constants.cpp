#include "drape_frontend/color_constants.hpp"

#include "base/assert.hpp"

#include "std/array.hpp"
#include "std/unordered_map.hpp"

namespace df
{

unordered_map<int, unordered_map<int, dp::Color>> kColorConstants =
{
  { MapStyleClear,
    {
      { DownloadButton, dp::Color(0, 0, 0, 0.44 * 255) },
      { DownloadButtonRouting, dp::Color(32, 152, 82, 255) },
      { DownloadButtonPressed, dp::Color(0, 0, 0, 0.72 * 255) },
      { DownloadButtonRoutingPressed, dp::Color(24, 128, 68, 255) },
      { GuiText, dp::Color(0x4D, 0x4D, 0x4D, 0xDD) },
      { MyPositionAccuracy, dp::Color(30, 150, 240, 20) },
      { Selection, dp::Color(0x17, 0xBF, 0x8E, 0x6F) },
      { Route, dp::Color(30, 150, 240, 204) },
      { RoutePedestrian, dp::Color(5, 105, 175, 204) },
    }
  },
  { MapStyleDark,
    {
      { DownloadButton, dp::Color(0, 0, 0, 0.44 * 255) },
      { DownloadButtonRouting, dp::Color(32, 152, 82, 255) },
      { DownloadButtonPressed, dp::Color(0, 0, 0, 0.72 * 255) },
      { DownloadButtonRoutingPressed, dp::Color(24, 128, 68, 255) },
      { GuiText, dp::Color(0x4D, 0x4D, 0x4D, 0xDD) },
      { MyPositionAccuracy, dp::Color(30, 150, 240, 20) },
      { Selection, dp::Color(0x17, 0xBF, 0x8E, 0x6F) },
      { Route, dp::Color(30, 150, 240, 204) },
      { RoutePedestrian, dp::Color(5, 105, 175, 204) },
    }
  },
};

dp::Color GetColorConstant(MapStyle style, ColorConstant constant)
{
  // "Light" and "clear" theme share the same colors.
  if (style == MapStyle::MapStyleLight)
    style = MapStyle::MapStyleClear;

  int const styleIndex = static_cast<int>(style);
  int const colorIndex = static_cast<int>(constant);

  ASSERT(kColorConstants.find(styleIndex) != kColorConstants.end(), ());
  ASSERT(kColorConstants[styleIndex].find(colorIndex) != kColorConstants[styleIndex].end(), ());

  return kColorConstants[styleIndex][colorIndex];
}

} // namespace df
