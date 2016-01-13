#pragma once

#include "drape/color.hpp"

#include "indexer/map_style.hpp"

namespace df
{

enum ColorConstant
{
  DownloadButton,
  DownloadButtonRouting,
  DownloadButtonPressed,
  DownloadButtonRoutingPressed,
  DownloadButtonText,
  CountryStatusText,
  GuiText,
  MyPositionAccuracy,
  Selection,
  Route,
  RoutePedestrian,
  Arrow3D,
  TrackHumanSpeed,
  TrackCarSpeed,
  TrackPlaneSpeed,
  TrackUnknownDistance
};

dp::Color GetColorConstant(MapStyle style, ColorConstant constant);

} // namespace df
