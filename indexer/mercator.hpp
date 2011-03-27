#pragma once
#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"
#include "../base/math.hpp"

struct MercatorBounds
{
  static double minX;
  static double maxX;
  static double minY;
  static double maxY;

  inline static double YToLat(double y)
  {
    return my::RadToDeg(2.0 * atan(exp(my::DegToRad(y))) - math::pi / 2.0);
  }

  inline static double LatToY(double lat)
  {
    lat = my::clamp(lat, -86.0, 86.0);
    double const res = my::RadToDeg(log(tan(my::DegToRad(45.0 + lat * 0.5))));
    return my::clamp(res, -180.0, 180.0);
  }

  inline static double XToLon(double x)
  {
    return x;
  }

  inline static double LonToX(double lon)
  {
    return lon;
  }

  static double const degreeInMetres;

  inline static double ConvertMetresToY(double lat, double metresValue)
  {
    return LatToY(lat + metresValue * degreeInMetres) - LatToY(lat);
  }

  inline static double ConvertMetresToX(double lon, double metresValue)
  {
    return LonToX(lon + metresValue * degreeInMetres) - LonToX(lon);
  }

  /// @return mercator bound points in rect
  inline static m2::RectD MetresToXY(double lon, double lat, double metresValue)
  {
    // We use approximate number of metres per degree
    double const degreeOffset = metresValue / 2.0 * degreeInMetres;
    return m2::RectD(LonToX(lon - degreeOffset),
                     LatToY(lat - degreeOffset),
                     LonToX(lon + degreeOffset),
                     LatToY(lat + degreeOffset));
  }

  static double GetCellID2PointAbsEpsilon() { return 1.0E-4; }
};
