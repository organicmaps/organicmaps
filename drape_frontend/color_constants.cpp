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
      { GuiText, dp::Color(77, 77, 77, 221) },
      { MyPositionAccuracy, dp::Color(0, 0, 0, 20) },
      { Selection, dp::Color(30, 150, 240, 164) },
      { Route, dp::Color(0, 135, 255, 255) },
      { RouteOutline, dp::Color(5, 95, 205, 255) },
      { RouteTrafficG0, dp::Color(155, 35, 0, 255) },
      { RouteTrafficG1, dp::Color(240, 60, 10, 255) },
      { RouteTrafficG2, dp::Color(240, 60, 10, 255) },
      { RouteTrafficG3, dp::Color(245, 215, 40, 127) },
      { RoutePedestrian, dp::Color(29, 51, 158, 204) },
      { RouteBicycle, dp::Color(156, 39, 176, 204) },
      { Arrow3D, dp::Color(80, 170, 255, 255) },
      { Arrow3DObsolete, dp::Color(130, 170, 200, 183) },
      { TrackHumanSpeed, dp::Color(29, 51, 158, 255) },
      { TrackCarSpeed, dp::Color(21, 121, 244, 255) },
      { TrackPlaneSpeed, dp::Color(10, 196, 255, 255) },
      { TrackUnknownDistance, dp::Color(97, 97, 97, 255) },
      { TrafficG0, dp::Color(155, 35, 5, 255) },
      { TrafficG1, dp::Color(230, 60, 55, 255) },
      { TrafficG2, dp::Color(230, 60, 55, 255) },
      { TrafficG3, dp::Color(250, 180, 35, 127) },
      { TrafficG4, dp::Color(155, 175, 55, 255) },
      { TrafficG5, dp::Color(70, 135, 85, 255) },
      { TrafficTempBlock, dp::Color(70, 70, 70, 255) },
      { TrafficUnknown, dp::Color(0, 0, 0, 0) },
      { TrafficArrowLight, dp::Color(255, 255, 255, 255) },
      { TrafficArrowDark, dp::Color(107, 81, 20, 255) },
      { TrafficOutline, dp::Color(255, 255, 255, 255) },
    }
  },
  { MapStyleDark,
    {
      { GuiText, dp::Color(255, 255, 255, 178) },
      { MyPositionAccuracy, dp::Color(255, 255, 255, 15) },
      { Selection, dp::Color(75, 185, 230, 164) },
      { Route, dp::Color(0, 135, 255, 255) },
      { RouteOutline, dp::Color(5, 95, 205, 255) },
      { RouteTrafficG0, dp::Color(155, 35, 0, 255) },
      { RouteTrafficG1, dp::Color(240, 60, 10, 255) },
      { RouteTrafficG2, dp::Color(240, 60, 10, 255) },
      { RouteTrafficG3, dp::Color(245, 215, 40, 127) },
      { RoutePedestrian, dp::Color(255, 185, 75, 180) },
      { RouteBicycle, dp::Color(255, 75, 140, 180) },
      { Arrow3D, dp::Color(65, 200, 255, 255) },
      { Arrow3DObsolete, dp::Color(130, 170, 200, 183) },
      { TrackHumanSpeed, dp::Color(255, 152, 0, 255) },
      { TrackCarSpeed, dp::Color(255, 202, 40, 255) },
      { TrackPlaneSpeed, dp::Color(255, 245, 160, 255) },
      { TrackUnknownDistance, dp::Color(150, 150, 150, 255) },
      { TrafficG0, dp::Color(65, 15, 0, 255) },
      { TrafficG1, dp::Color(120, 25, 25, 255) },
      { TrafficG2, dp::Color(120, 25, 25, 255) },
      { TrafficG3, dp::Color(140, 90, 0, 127) },
      { TrafficG4, dp::Color(85, 85, 25, 255) },
      { TrafficG5, dp::Color(40, 80, 50, 255) },
      { TrafficTempBlock, dp::Color(35, 35, 35, 255) },
      { TrafficUnknown, dp::Color(0, 0, 0, 0) },
      { TrafficArrowLight, dp::Color(170, 170, 170, 255) },
      { TrafficArrowDark, dp::Color(30, 30, 30, 255) },
      { TrafficOutline, dp::Color(0, 0, 0, 255) },
    }
  },
};

dp::Color GetColorConstant(MapStyle style, ColorConstant constant)
{
  int const styleIndex = static_cast<int>(style);
  int const colorIndex = static_cast<int>(constant);

  ASSERT(kColorConstants.find(styleIndex) != kColorConstants.end(), ());
  ASSERT(kColorConstants[styleIndex].find(colorIndex) != kColorConstants[styleIndex].end(), ());

  return kColorConstants[styleIndex][colorIndex];
}

} // namespace df
