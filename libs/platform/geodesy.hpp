#pragma once

#include "geometry/latlon.hpp"

#include "base/math.hpp"

#include <cmath>

/* Shared geodesy for the national grid systems (OS Grid, Irish Grid, ITM).
 *
 * These systems differ only in their ellipsoid, datum shift and transverse Mercator parameters; the
 * underlying maths - the 7-parameter Helmert datum transform and the Lee-Redfearn transverse Mercator
 * projection - is identical. It lives here once, parameterised by Ellipsoid / HelmertParams / TMParams,
 * so each grid file keeps only its own constants plus its grid-lettering and range rules.
 *
 * Header-only on purpose: the functions are small and leaf, and a header avoids adding a new compiled
 * translation unit to both CMake and the Xcode project.
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

inline Cartesian GeodeticToCartesian(ms::LatLon const & ll, Ellipsoid const & e)
{
  double const phi = math::DegToRad(ll.m_lat);
  double const lambda = math::DegToRad(ll.m_lon);
  double const sinPhi = std::sin(phi);
  double const cosPhi = std::cos(phi);
  double const eSq = e.EccentricitySq();
  double const nu = e.a / std::sqrt(1.0 - eSq * sinPhi * sinPhi);  // Height is assumed zero.

  return {nu * cosPhi * std::cos(lambda), nu * cosPhi * std::sin(lambda), (1.0 - eSq) * nu * sinPhi};
}

inline ms::LatLon CartesianToGeodetic(Cartesian const & c, Ellipsoid const & e)
{
  double const eSq = e.EccentricitySq();
  double const p = std::sqrt(c.x * c.x + c.y * c.y);
  double phi = std::atan2(c.z, p * (1.0 - eSq));

  // Iterate to convergence (latitude depends on the radius of curvature, which depends on latitude).
  for (int i = 0; i < 10; ++i)
  {
    double const sinPhi = std::sin(phi);
    double const nu = e.a / std::sqrt(1.0 - eSq * sinPhi * sinPhi);
    double const phiNew = std::atan2(c.z + eSq * nu * sinPhi, p);
    if (std::fabs(phiNew - phi) < 1e-12)
    {
      phi = phiNew;
      break;
    }
    phi = phiNew;
  }

  return {math::RadToDeg(phi), math::RadToDeg(std::atan2(c.y, c.x))};
}

inline Cartesian ApplyHelmert(Cartesian const & c, HelmertParams const & h)
{
  double const scale = 1.0 + h.s;
  return {h.tx + scale * c.x - h.rz * c.y + h.ry * c.z, h.ty + h.rz * c.x + scale * c.y - h.rx * c.z,
          h.tz - h.ry * c.x + h.rx * c.y + scale * c.z};
}

// Shift a lat/lon from one datum/ellipsoid to another via a Helmert transform (height assumed zero).
inline ms::LatLon ShiftDatum(ms::LatLon const & ll, Ellipsoid const & from, HelmertParams const & h,
                             Ellipsoid const & to)
{
  return CartesianToGeodetic(ApplyHelmert(GeodeticToCartesian(ll, from), h), to);
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

// Meridional arc from the true origin to latitude phi for the given projection. Used by both directions.
inline double MeridionalArc(double phi, TMParams const & tm)
{
  double const n = tm.ellipsoid.ThirdFlattening();
  double const n2 = n * n;
  double const n3 = n2 * n;
  double const dMinus = phi - tm.lat0;
  double const dPlus = phi + tm.lat0;

  return tm.ellipsoid.b * tm.F0 *
         ((1.0 + n + 5.0 / 4.0 * n2 + 5.0 / 4.0 * n3) * dMinus -
          (3.0 * n + 3.0 * n2 + 21.0 / 8.0 * n3) * std::sin(dMinus) * std::cos(dPlus) +
          (15.0 / 8.0 * n2 + 15.0 / 8.0 * n3) * std::sin(2.0 * dMinus) * std::cos(2.0 * dPlus) -
          35.0 / 24.0 * n3 * std::sin(3.0 * dMinus) * std::cos(3.0 * dPlus));
}

// Radii of curvature (x scale factor) and eta^2 at latitude phi. Shared by both projection directions.
struct Curvature
{
  double nu;    // Transverse radius of curvature.
  double rho;   // Meridional radius of curvature.
  double eta2;  // nu / rho - 1.
};

inline Curvature ComputeCurvature(double sinPhi, TMParams const & tm)
{
  double const eSq = tm.ellipsoid.EccentricitySq();
  double const t = 1.0 - eSq * sinPhi * sinPhi;
  double const sqrtT = std::sqrt(t);
  double const nu = tm.ellipsoid.a * tm.F0 / sqrtT;
  double const rho = tm.ellipsoid.a * tm.F0 * (1.0 - eSq) / (t * sqrtT);
  return {nu, rho, nu / rho - 1.0};
}

// lat/lon (on the projection's own datum) -> easting/northing (Lee-Redfearn transverse Mercator).
inline EN LatLonToEN(ms::LatLon const & ll, TMParams const & tm)
{
  double const phi = math::DegToRad(ll.m_lat);
  double const lambda = math::DegToRad(ll.m_lon);
  double const sinPhi = std::sin(phi);
  double const cosPhi = std::cos(phi);
  double const tanPhi = sinPhi / cosPhi;
  double const tan2 = tanPhi * tanPhi;
  double const tan4 = tan2 * tan2;

  auto const [nu, rho, eta2] = ComputeCurvature(sinPhi, tm);

  double const cos3 = cosPhi * cosPhi * cosPhi;
  double const cos5 = cos3 * cosPhi * cosPhi;

  double const I = MeridionalArc(phi, tm) + tm.N0;
  double const II = nu / 2.0 * sinPhi * cosPhi;
  double const III = nu / 24.0 * sinPhi * cos3 * (5.0 - tan2 + 9.0 * eta2);
  double const IIIA = nu / 720.0 * sinPhi * cos5 * (61.0 - 58.0 * tan2 + tan4);
  double const IV = nu * cosPhi;
  double const V = nu / 6.0 * cos3 * (nu / rho - tan2);
  double const VI = nu / 120.0 * cos5 * (5.0 - 18.0 * tan2 + tan4 + 14.0 * eta2 - 58.0 * tan2 * eta2);

  double const dLon = lambda - tm.lon0;
  double const dLon2 = dLon * dLon;
  double const dLon3 = dLon2 * dLon;
  double const dLon4 = dLon2 * dLon2;
  double const dLon5 = dLon4 * dLon;
  double const dLon6 = dLon3 * dLon3;

  double const northing = I + II * dLon2 + III * dLon4 + IIIA * dLon6;
  double const easting = tm.E0 + IV * dLon + V * dLon3 + VI * dLon5;
  return {easting, northing};
}

// easting/northing -> lat/lon on the projection's own datum (inverse Lee-Redfearn).
inline ms::LatLon ENToLatLon(EN const & en, TMParams const & tm)
{
  // Iterate the latitude until the meridional arc matches the requested northing. For any in-range
  // northing this converges in a handful of steps; the fixed iteration cap is a safety net so a
  // pathological input cannot spin forever (mirrors the capped CartesianToGeodetic loop above).
  double phi = tm.lat0;
  double m = 0.0;
  for (int i = 0; i < 20; ++i)
  {
    phi += (en.northing - tm.N0 - m) / (tm.ellipsoid.a * tm.F0);
    m = MeridionalArc(phi, tm);
    if (std::fabs(en.northing - tm.N0 - m) < 0.00001)  // 0.01 mm.
      break;
  }

  double const sinPhi = std::sin(phi);
  double const cosPhi = std::cos(phi);
  double const tanPhi = sinPhi / cosPhi;
  double const tan2 = tanPhi * tanPhi;
  double const tan4 = tan2 * tan2;
  double const tan6 = tan4 * tan2;

  auto const [nu, rho, eta2] = ComputeCurvature(sinPhi, tm);

  double const nu3 = nu * nu * nu;
  double const nu5 = nu3 * nu * nu;
  double const nu7 = nu5 * nu * nu;
  double const secPhi = 1.0 / cosPhi;

  double const VII = tanPhi / (2.0 * rho * nu);
  double const VIII = tanPhi / (24.0 * rho * nu3) * (5.0 + 3.0 * tan2 + eta2 - 9.0 * tan2 * eta2);
  double const IX = tanPhi / (720.0 * rho * nu5) * (61.0 + 90.0 * tan2 + 45.0 * tan4);
  double const X = secPhi / nu;
  double const XI = secPhi / (6.0 * nu3) * (nu / rho + 2.0 * tan2);
  double const XII = secPhi / (120.0 * nu5) * (5.0 + 28.0 * tan2 + 24.0 * tan4);
  double const XIIA = secPhi / (5040.0 * nu7) * (61.0 + 662.0 * tan2 + 1320.0 * tan4 + 720.0 * tan6);

  double const dE = en.easting - tm.E0;
  double const dE2 = dE * dE;
  double const dE3 = dE2 * dE;
  double const dE4 = dE2 * dE2;
  double const dE5 = dE4 * dE;
  double const dE6 = dE3 * dE3;
  double const dE7 = dE6 * dE;

  double const lat = phi - VII * dE2 + VIII * dE4 - IX * dE6;
  double const lon = tm.lon0 + X * dE - XI * dE3 + XII * dE5 - XIIA * dE7;
  return {math::RadToDeg(lat), math::RadToDeg(lon)};
}
}  // namespace geodesy
