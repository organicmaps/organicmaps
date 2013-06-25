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

  inline static m2::RectD FullRect() { return m2::RectD(minX, minY, maxX, maxY); }

  inline static bool ValidLon(double d)
  {
    return my::between_s(-180.0, 180.0, d);
  }
  inline static bool ValidLat(double d)
  {
    return my::between_s(-90.0, 90.0, d);
  }

  inline static bool ValidX(double d)
  {
    return my::between_s(minX, maxX, d);
  }
  inline static bool ValidY(double d)
  {
    return my::between_s(minY, maxY, d);
  }

  inline static double ClampX(double d)
  {
    return my::clamp(d, minX, maxX);
  }
  inline static double ClampY(double d)
  {
    return my::clamp(d, minY, maxY);
  }

  inline static double YToLat(double y)
  {
    return my::RadToDeg(2.0 * atan(exp(my::DegToRad(y))) - math::pi / 2.0);
  }

  inline static double LatToY(double lat)
  {
    lat = my::clamp(lat, -86.0, 86.0);
    double const res = my::RadToDeg(log(tan(my::DegToRad(45.0 + lat * 0.5))));
    return ClampY(res);
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

  static m2::PointD GetSmPoint(m2::PointD const & pt, double lonMetresR, double latMetresR);

  static double GetCellID2PointAbsEpsilon() { return 1.0E-4; }

  /// Converts lat lon rect to mercator one
  inline static m2::RectD FromLatLonRect(m2::RectD const & latLonRect)
  {
    return m2::RectD(LonToX(latLonRect.minX()), LatToY(latLonRect.minY()),
                     LonToX(latLonRect.maxX()), LatToY(latLonRect.maxY()));

  }
};
