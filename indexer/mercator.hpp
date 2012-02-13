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

  inline static double ClampX(double d)
  {
    if (d < MercatorBounds::minX) return MercatorBounds::minX;
    if (d > MercatorBounds::maxX) return MercatorBounds::maxX;
    return d;
  }

  inline static double ClampY(double d)
  {
    if (d < MercatorBounds::minY) return MercatorBounds::minY;
    if (d > MercatorBounds::maxY) return MercatorBounds::maxY;
    return d;
  }

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

  /// @name Get rect for center point (lon, lat) and dimensions in metres.
  //@{
  /// @return mercator rect.
  static m2::RectD MetresToXY(double lon, double lat,
                              double lonMetresR, double latMetresR);

  inline static m2::RectD MetresToXY(double lon, double lat, double metresR)
  {
    return MetresToXY(lon, lat, metresR, metresR);
  }
  //@}

  inline static m2::RectD RectByCenterXYAndSizeInMeters(double centerX, double centerY,
                                                        double sizeX, double sizeY)
  {
    ASSERT_GREATER_OR_EQUAL(sizeX, 0, ());
    ASSERT_GREATER_OR_EQUAL(sizeY, 0, ());

    return MetresToXY(XToLon(centerX), YToLat(centerY), sizeX, sizeY);
  }

  inline static m2::RectD RectByCenterXYAndSizeInMeters(m2::PointD const & center, double size)
  {
    return RectByCenterXYAndSizeInMeters(center.x, center.y, size, size);
  }

  static double GetCellID2PointAbsEpsilon() { return 1.0E-4; }
};
