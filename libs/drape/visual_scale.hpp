#pragma once
#include "platform/platform.hpp"

namespace dp
{
/// This fuction is called in iOS/Android native code.
inline double VisualScale(double exactDensityDPI)
{
  // In case of tablets and iPads increased DPI is used to make visual scale bigger.
  if (GetPlatform().IsTablet())
    exactDensityDPI *= 1.2;

  // For some old devices (for example iPad 2) the density could be less than 160 DPI (mdpi).
  // Returns one in that case to keep readable text on the map.
  return std::max(1.35, exactDensityDPI / 160.0);
}
}  //  namespace dp
