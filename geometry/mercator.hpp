#pragma once

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/math.hpp"

struct MercatorBounds
{
  static double constexpr kMinX = -180.0;
  static double constexpr kMaxX = 180.0;
  static double constexpr kMinY = -180.0;
  static double constexpr kMaxY = 180.0;
  static double constexpr kRangeX = kMaxX - kMinX;
  static double constexpr kRangeY = kMaxY - kMinY;

  // The denominator is the Earth circumference at the Equator in meters.
  // The value is a bit off for some reason; 40075160 seems to be correct.
  static double constexpr kDegreesInMeter = 360.0 / 40008245.0;
  static double constexpr kMetersInDegree = 40008245.0 / 360.0;

  static m2::RectD FullRect() { return m2::RectD(kMinX, kMinY, kMaxX, kMaxY); }

  static bool ValidLon(double d) { return base::between_s(-180.0, 180.0, d); }
  static bool ValidLat(double d) { return base::between_s(-90.0, 90.0, d); }

  static bool ValidX(double d) { return base::between_s(kMinX, kMaxX, d); }
  static bool ValidY(double d) { return base::between_s(kMinY, kMaxY, d); }

  static double ClampX(double d) { return base::clamp(d, kMinX, kMaxX); }
  static double ClampY(double d) { return base::clamp(d, kMinY, kMaxY); }

  static double YToLat(double y) { return base::RadToDeg(2.0 * atan(tanh(0.5 * base::DegToRad(y)))); }

  static double LatToY(double lat)
  {
    double const sinx = sin(base::DegToRad(base::clamp(lat, -86.0, 86.0)));
    double const res = base::RadToDeg(0.5 * log((1.0 + sinx) / (1.0 - sinx)));
    return ClampY(res);
  }

  static double XToLon(double x) { return x; }

  static double LonToX(double lon) { return lon; }

  static double MetersToMercator(double meters) { return meters * kDegreesInMeter; }
  static double MercatorToMeters(double mercator) { return mercator * kMetersInDegree; }

  /// @name Get rect for center point (lon, lat) and dimensions in metres.
  //@{
  /// @return mercator rect.
  static m2::RectD MetersToXY(double lon, double lat, double lonMetersR, double latMetersR);

  static m2::RectD MetersToXY(double lon, double lat, double metresR)
  {
    return MetersToXY(lon, lat, metresR, metresR);
  }
  //@}

  static m2::RectD RectByCenterXYAndSizeInMeters(double centerX, double centerY, double sizeX,
                                                 double sizeY)
  {
    ASSERT_GREATER_OR_EQUAL(sizeX, 0, ());
    ASSERT_GREATER_OR_EQUAL(sizeY, 0, ());

    return MetersToXY(XToLon(centerX), YToLat(centerY), sizeX, sizeY);
  }

  static m2::RectD RectByCenterXYAndSizeInMeters(m2::PointD const & center, double size)
  {
    return RectByCenterXYAndSizeInMeters(center.x, center.y, size, size);
  }

  static m2::RectD RectByCenterXYAndOffset(m2::PointD const & center, double offset)
  {
    return {ClampX(center.x - offset), ClampY(center.y - offset),
            ClampX(center.x + offset), ClampY(center.y + offset)};
  }

  static m2::PointD GetSmPoint(m2::PointD const & pt, double lonMetersR, double latMetersR);

  static m2::PointD FromLatLon(double lat, double lon)
  {
    return m2::PointD(LonToX(lon), LatToY(lat));
  }

  static m2::PointD FromLatLon(ms::LatLon const & point)
  {
    return FromLatLon(point.lat, point.lon);
  }

  static m2::RectD RectByCenterLatLonAndSizeInMeters(double lat, double lon, double size)
  {
    return RectByCenterXYAndSizeInMeters(FromLatLon(lat, lon), size);
  }

  static ms::LatLon ToLatLon(m2::PointD const & point)
  {
    return {YToLat(point.y), XToLon(point.x)};
  }

  /// Converts lat lon rect to mercator one
  static m2::RectD FromLatLonRect(m2::RectD const & latLonRect)
  {
    return m2::RectD(FromLatLon(latLonRect.minY(), latLonRect.minX()),
                     FromLatLon(latLonRect.maxY(), latLonRect.maxX()));
  }

  static m2::RectD ToLatLonRect(m2::RectD const & mercatorRect)
  {
    return m2::RectD(YToLat(mercatorRect.minY()), XToLon(mercatorRect.minX()),
                     YToLat(mercatorRect.maxY()), XToLon(mercatorRect.maxX()));
  }

  /// Calculates distance on Earth in meters between two mercator points.
  static double DistanceOnEarth(m2::PointD const & p1, m2::PointD const & p2);

  /// Calculates area of a triangle on Earth in m² by three mercator points.
  static double AreaOnEarth(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
};
