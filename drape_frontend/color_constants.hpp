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
  GuiText,
  MyPositionAccuracy,
  Selection,
  Route,
  RoutePedestrian,
};

dp::Color GetColorConstant(MapStyle style, ColorConstant constant);

} // namespace df
