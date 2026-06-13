#include "geometry/geodesy.hpp"

#include "base/math.hpp"

#include <cmath>

namespace geodesy
{
Cartesian GeodeticToCartesian(ms::LatLon const & ll, Ellipsoid const & e)
{
  double const phi = math::DegToRad(ll.m_lat);
  double const lambda = math::DegToRad(ll.m_lon);
  double const sinPhi = std::sin(phi);
  double const cosPhi = std::cos(phi);
  double const eSq = e.EccentricitySq();
  double const nu = e.a / std::sqrt(1.0 - eSq * sinPhi * sinPhi);  // Height is assumed zero.

  return {nu * cosPhi * std::cos(lambda), nu * cosPhi * std::sin(lambda), (1.0 - eSq) * nu * sinPhi};
}

ms::LatLon CartesianToGeodetic(Cartesian const & c, Ellipsoid const & e)
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

Cartesian ApplyHelmert(Cartesian const & c, HelmertParams const & h)
{
  double const scale = 1.0 + h.s;
  return {h.tx + scale * c.x - h.rz * c.y + h.ry * c.z, h.ty + h.rz * c.x + scale * c.y - h.rx * c.z,
          h.tz - h.ry * c.x + h.rx * c.y + scale * c.z};
}

ms::LatLon ShiftDatum(ms::LatLon const & ll, Ellipsoid const & from, HelmertParams const & h, Ellipsoid const & to)
{
  return CartesianToGeodetic(ApplyHelmert(GeodeticToCartesian(ll, from), h), to);
}

double MeridionalArc(double phi, TMParams const & tm)
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

Curvature ComputeCurvature(double sinPhi, TMParams const & tm)
{
  double const eSq = tm.ellipsoid.EccentricitySq();
  double const t = 1.0 - eSq * sinPhi * sinPhi;
  double const sqrtT = std::sqrt(t);
  double const nu = tm.ellipsoid.a * tm.F0 / sqrtT;
  double const rho = tm.ellipsoid.a * tm.F0 * (1.0 - eSq) / (t * sqrtT);
  return {nu, rho, nu / rho - 1.0};
}

EN LatLonToEN(ms::LatLon const & ll, TMParams const & tm)
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

ms::LatLon ENToLatLon(EN const & en, TMParams const & tm)
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
