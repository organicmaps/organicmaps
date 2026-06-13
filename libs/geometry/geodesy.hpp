#pragma once

#include "geometry/latlon.hpp"

#include "base/math.hpp"

/* Shared geodesy for the national grid systems (OS Grid, Irish Grid, ITM).
 *
 * These systems differ only in their ellipsoid, datum shift and transverse Mercator parameters; the
 * underlying maths - the 7-parameter Helmert datum transform and the Lee-Redfearn transverse Mercator
 * projection - is identical. It lives here once, parameterised by Ellipsoid / HelmertParams / TMParams,
 * so each grid file keeps only its own constants plus its grid-lettering and range rules.
 */
namespace geodesy
{
struct Ellipsoid
{
  double a;  // Semi-major axis, m.
  double b;  // Semi-minor axis, m.
  double constexpr EccentricitySq() const { return (a * a - b * b) / (a * a); }
  double constexpr ThirdFlattening() const { return (a - b) / (a + b); }
};

// WGS84. Also stands in for GRS80/ETRS89, which differ from it by well under a millimetre here.
Ellipsoid constexpr kWgs84{6378137.0, 6356752.314245};

struct Cartesian
{
  double x, y, z;
};

// 7-parameter Helmert transform. Rotations are stored in radians, scale as a ratio.
struct HelmertParams
{
  double tx, ty, tz;  // Translations, m.
  double rx, ry, rz;  // Rotations, rad.
  double s;           // Scale, ratio.
};

double constexpr SecToRad(double sec)
{
  return math::DegToRad(sec / 3600.0);
}

// The reverse transform. Negating the linearized parameters is the standard approximation (round-trip
// error < 1 cm, far below the metre-level accuracy of the Helmert transforms themselves).
HelmertParams constexpr InverseHelmert(HelmertParams const & h)
{
  return {-h.tx, -h.ty, -h.tz, -h.rx, -h.ry, -h.rz, -h.s};
}

// A transverse Mercator definition: ellipsoid + projection origin, scale and false origin.
struct TMParams
{
  Ellipsoid ellipsoid;
  double F0;    // Scale factor on the central meridian.
  double lat0;  // True origin latitude, rad.
  double lon0;  // True origin longitude, rad.
  double E0;    // False easting, m.
  double N0;    // False northing, m.
};

struct EN
{
  double easting, northing;
};

// Radii of curvature (x scale factor) and eta^2 at latitude phi. Shared by both projection directions.
struct Curvature
{
  double nu;    // Transverse radius of curvature.
  double rho;   // Meridional radius of curvature.
  double eta2;  // nu / rho - 1.
};

Cartesian GeodeticToCartesian(ms::LatLon const & ll, Ellipsoid const & e);
ms::LatLon CartesianToGeodetic(Cartesian const & c, Ellipsoid const & e);
Cartesian ApplyHelmert(Cartesian const & c, HelmertParams const & h);

// Shift a lat/lon from one datum/ellipsoid to another via a Helmert transform (height assumed zero).
ms::LatLon ShiftDatum(ms::LatLon const & ll, Ellipsoid const & from, HelmertParams const & h, Ellipsoid const & to);

// Meridional arc from the true origin to latitude phi for the given projection. Used by both directions.
double MeridionalArc(double phi, TMParams const & tm);

Curvature ComputeCurvature(double sinPhi, TMParams const & tm);

// lat/lon (on the projection's own datum) -> easting/northing (Lee-Redfearn transverse Mercator).
EN LatLonToEN(ms::LatLon const & ll, TMParams const & tm);

// easting/northing -> lat/lon on the projection's own datum (inverse Lee-Redfearn).
ms::LatLon ENToLatLon(EN const & en, TMParams const & tm);
}  // namespace geodesy
