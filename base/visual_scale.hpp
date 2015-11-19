#pragma once

inline double VisualScale(double exactDensityDPI)
{
  double const mdpiDensityDPI = 160.;
  // For some old devices (for example iPad 2) the density could be less than 160 DPI.
  // Returns one in that case to keep readable text on the map.
  if (exactDensityDPI <= mdpiDensityDPI)
    return 1.;
  return exactDensityDPI / mdpiDensityDPI;
}
