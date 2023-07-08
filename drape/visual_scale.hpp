#pragma once

namespace dp
{
inline double VisualScale(double exactDensityDPI)
{
  double constexpr kMdpiDensityDPI = 160.;
  double const tabletFactor = 1.2;
  // In case of tablets and iPads increased DPI is used to make visual scale bigger.
  if (GetPlatform().IsTablet())
    exactDensityDPI *= tabletFactor;

  // For some old devices (for example iPad 2) the density could be less than 160 DPI.
  // Returns one in that case to keep readable text on the map.
  return std::max(1.35, exactDensityDPI / kMdpiDensityDPI);
}
} //  namespace dp
