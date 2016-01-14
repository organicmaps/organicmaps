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
      { DownloadButton, dp::Color(0, 0, 0, 112) },
      { DownloadButtonRouting, dp::Color(32, 152, 82, 255) },
      { DownloadButtonPressed, dp::Color(0, 0, 0, 184) },
      { DownloadButtonRoutingPressed, dp::Color(24, 128, 68, 255) },
      { DownloadButtonText, dp::Color(255, 255, 255, 255) },
      { CountryStatusText, dp::Color(0, 0, 0, 255) },
      { GuiText, dp::Color(77, 77, 77, 221) },
      { MyPositionAccuracy, dp::Color(30, 150, 240, 20) },
      { Selection, dp::Color(30, 150, 240, 164) },
      { Route, dp::Color(21, 121, 244, 204) },
      { RoutePedestrian, dp::Color(29, 51, 158, 204) },
      { Arrow3D, dp::Color(30, 150, 240, 255) },
      { TrackHumanSpeed, dp::Color(29, 51, 158, 255) },
      { TrackCarSpeed, dp::Color(21, 121, 244, 255) },
      { TrackPlaneSpeed, dp::Color(10, 196, 255, 255) },
      { TrackUnknownDistance, dp::Color(97, 97, 97, 255) },
    }
  },
  { MapStyleDark,
    {
      { DownloadButton, dp::Color(255, 255, 255, 178) },
      { DownloadButtonRouting, dp::Color(255, 230, 140, 255) },
      { DownloadButtonPressed, dp::Color(255, 255, 255, 77) },
      { DownloadButtonRoutingPressed, dp::Color(200, 180, 110, 255) },
      { DownloadButtonText, dp::Color(0, 0, 0, 222) },
      { CountryStatusText, dp::Color(255, 255, 255, 222) },
      { GuiText, dp::Color(255, 255, 255, 178) },
      { MyPositionAccuracy, dp::Color(255, 230, 140, 20) },
      { Selection, dp::Color(255, 230, 140, 164) },
      { Route, dp::Color(255, 202, 40, 180) },
      { RoutePedestrian, dp::Color(255, 152, 0, 180) },
      { Arrow3D, dp::Color(255, 230, 140, 255) },
      { TrackHumanSpeed, dp::Color(255, 152, 0, 255) },
      { TrackCarSpeed, dp::Color(255, 202, 40, 255) },
      { TrackPlaneSpeed, dp::Color(255, 245, 160, 255) },
      { TrackUnknownDistance, dp::Color(150, 150, 150, 255) },
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
