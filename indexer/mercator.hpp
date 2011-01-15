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

  /// @return mercator bound points in rect
  inline static m2::RectD ErrorToRadius(double lon, double lat, double errorInMetres)
  {
    // We use approximate number of metres per degree
    double const offset = errorInMetres / 2.0 * degreeInMetres;
    return m2::RectD(LonToX(lon - offset), LatToY(lat - offset), LonToX(lon + offset), LatToY(lat + offset));
  }

  static double GetCellID2PointAbsEpsilon() { return 1.0E-4; }
};
