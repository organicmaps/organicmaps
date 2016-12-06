#pragma once

#include "drape/color.hpp"

#include "indexer/map_style.hpp"

namespace df
{

enum ColorConstant
{
  GuiText,
  MyPositionAccuracy,
  Selection,
  Route,
  RouteOutline,
  RoutePedestrian,
  RouteBicycle,
  Arrow3D,
  Arrow3DObsolete,
  TrackHumanSpeed,
  TrackCarSpeed,
  TrackPlaneSpeed,
  TrackUnknownDistance,
  TrafficG0,
  TrafficG1,
  TrafficG2,
  TrafficG3,
  TrafficG4,
  TrafficG5,
  TrafficTempBlock,
  TrafficUnknown,
  TrafficArrowLight,
  TrafficArrowDark,
  TrafficOutline
};

dp::Color GetColorConstant(MapStyle style, ColorConstant constant);

} // namespace df
