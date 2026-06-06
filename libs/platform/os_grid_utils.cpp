#include "platform/os_grid_utils.hpp"

#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <cmath>

namespace os_grid_utils
{
using math::DegToRad;
using math::RadToDeg;

namespace
{
struct Ellipsoid
{
  double a;  // Semi-major axis, m.
  double b;  // Semi-minor axis, m.
  double constexpr EccentricitySq() const { return (a * a - b * b) / (a * a); }
};

Ellipsoid constexpr kWgs84{6378137.0, 6356752.314245};
Ellipsoid constexpr kAiry1830{6377563.396, 6356256.909};

// Airy 1830 constants shared by both projection directions.
double constexpr kAiryESq = kAiry1830.EccentricitySq();
double constexpr kAiryN = (kAiry1830.a - kAiry1830.b) / (kAiry1830.a + kAiry1830.b);

// National Grid transverse Mercator parameters (OSGB36 / Airy 1830).
double constexpr kF0 = 0.9996012717;      // Scale factor on the central meridian.
double constexpr kLat0 = DegToRad(49.0);  // True origin latitude  49°N.
double constexpr kLon0 = DegToRad(-2.0);  // True origin longitude  2°W.
double constexpr kE0 = 400000.0;          // False easting, m.
double constexpr kN0 = -100000.0;         // False northing, m.

// National Grid extent, metres: a valid two-letter square exists only within [0, 700 km) x [0, 1300 km).
double constexpr kMaxEasting = 700000.0;
double constexpr kMaxNorthing = 1300000.0;

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
  return DegToRad(sec / 3600.0);
}

HelmertParams constexpr InverseHelmert(HelmertParams const & h)
{
  return {-h.tx, -h.ty, -h.tz, -h.rx, -h.ry, -h.rz, -h.s};
}

// WGS84 -> OSGB36. Source: Ordnance Survey "A guide to coordinate systems in Great Britain".
HelmertParams constexpr kWgs84ToOsgb36{-446.448,          125.157,           -542.060,  SecToRad(-0.1502),
                                       SecToRad(-0.2470), SecToRad(-0.8421), 20.4894e-6};
// OSGB36 -> WGS84 is the inverse. Negating the parameters is the standard approximation for the
// linearized transform (round-trip error < 1 cm, far below the ~5 m Helmert accuracy).
HelmertParams constexpr kOsgb36ToWgs84 = InverseHelmert(kWgs84ToOsgb36);

Cartesian GeodeticToCartesian(ms::LatLon const & ll, Ellipsoid const & e)
{
  double const phi = DegToRad(ll.m_lat);
  double const lambda = DegToRad(ll.m_lon);
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

  return {RadToDeg(phi), RadToDeg(std::atan2(c.y, c.x))};
}

Cartesian ApplyHelmert(Cartesian const & c, HelmertParams const & h)
{
  double const scale = 1.0 + h.s;
  return {h.tx + scale * c.x - h.rz * c.y + h.ry * c.z, h.ty + h.rz * c.x + scale * c.y - h.rx * c.z,
          h.tz - h.ry * c.x + h.rx * c.y + scale * c.z};
}

ms::LatLon Wgs84ToOsgb36(ms::LatLon const & wgs)
{
  return CartesianToGeodetic(ApplyHelmert(GeodeticToCartesian(wgs, kWgs84), kWgs84ToOsgb36), kAiry1830);
}

ms::LatLon Osgb36ToWgs84(ms::LatLon const & osgb)
{
  return CartesianToGeodetic(ApplyHelmert(GeodeticToCartesian(osgb, kAiry1830), kOsgb36ToWgs84), kWgs84);
}

struct EN
{
  double easting, northing;
};

// Meridional arc from the true origin to latitude phi (Airy 1830). Used by both projection directions.
double MeridionalArc(double phi)
{
  double constexpr n2 = kAiryN * kAiryN;
  double constexpr n3 = n2 * kAiryN;
  double const dMinus = phi - kLat0;
  double const dPlus = phi + kLat0;

  return kAiry1830.b * kF0 *
         ((1.0 + kAiryN + 5.0 / 4.0 * n2 + 5.0 / 4.0 * n3) * dMinus -
          (3.0 * kAiryN + 3.0 * n2 + 21.0 / 8.0 * n3) * std::sin(dMinus) * std::cos(dPlus) +
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

Curvature ComputeCurvature(double sinPhi)
{
  double const t = 1.0 - kAiryESq * sinPhi * sinPhi;
  double const sqrtT = std::sqrt(t);
  double const nu = kAiry1830.a * kF0 / sqrtT;
  double const rho = kAiry1830.a * kF0 * (1.0 - kAiryESq) / (t * sqrtT);
  return {nu, rho, nu / rho - 1.0};
}

// OSGB36 lat/lon -> National Grid easting/northing (Lee-Redfearn transverse Mercator).
EN Osgb36ToEN(ms::LatLon const & osgb)
{
  double const phi = DegToRad(osgb.m_lat);
  double const lambda = DegToRad(osgb.m_lon);
  double const sinPhi = std::sin(phi);
  double const cosPhi = std::cos(phi);
  double const tanPhi = sinPhi / cosPhi;
  double const tan2 = tanPhi * tanPhi;
  double const tan4 = tan2 * tan2;

  auto const [nu, rho, eta2] = ComputeCurvature(sinPhi);

  double const cos3 = cosPhi * cosPhi * cosPhi;
  double const cos5 = cos3 * cosPhi * cosPhi;

  double const I = MeridionalArc(phi) + kN0;
  double const II = nu / 2.0 * sinPhi * cosPhi;
  double const III = nu / 24.0 * sinPhi * cos3 * (5.0 - tan2 + 9.0 * eta2);
  double const IIIA = nu / 720.0 * sinPhi * cos5 * (61.0 - 58.0 * tan2 + tan4);
  double const IV = nu * cosPhi;
  double const V = nu / 6.0 * cos3 * (nu / rho - tan2);
  double const VI = nu / 120.0 * cos5 * (5.0 - 18.0 * tan2 + tan4 + 14.0 * eta2 - 58.0 * tan2 * eta2);

  double const dLon = lambda - kLon0;
  double const dLon2 = dLon * dLon;
  double const dLon3 = dLon2 * dLon;
  double const dLon4 = dLon2 * dLon2;
  double const dLon5 = dLon4 * dLon;
  double const dLon6 = dLon3 * dLon3;

  double const northing = I + II * dLon2 + III * dLon4 + IIIA * dLon6;
  double const easting = kE0 + IV * dLon + V * dLon3 + VI * dLon5;
  return {easting, northing};
}

// National Grid easting/northing -> OSGB36 lat/lon (inverse Lee-Redfearn).
ms::LatLon ENToOsgb36(EN const & en)
{
  // Iterate the latitude until the meridional arc matches the requested northing.
  double phi = kLat0;
  double m = 0.0;
  do
  {
    phi += (en.northing - kN0 - m) / (kAiry1830.a * kF0);
    m = MeridionalArc(phi);
  }
  while (std::fabs(en.northing - kN0 - m) >= 0.00001);  // 0.01 mm.

  double const sinPhi = std::sin(phi);
  double const cosPhi = std::cos(phi);
  double const tanPhi = sinPhi / cosPhi;
  double const tan2 = tanPhi * tanPhi;
  double const tan4 = tan2 * tan2;
  double const tan6 = tan4 * tan2;

  auto const [nu, rho, eta2] = ComputeCurvature(sinPhi);

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

  double const dE = en.easting - kE0;
  double const dE2 = dE * dE;
  double const dE3 = dE2 * dE;
  double const dE4 = dE2 * dE2;
  double const dE5 = dE4 * dE;
  double const dE6 = dE3 * dE3;
  double const dE7 = dE6 * dE;

  double const lat = phi - VII * dE2 + VIII * dE4 - IX * dE6;
  double const lon = kLon0 + X * dE - XI * dE3 + XII * dE5 - XIIA * dE7;
  return {RadToDeg(lat), RadToDeg(lon)};
}

// easting/northing -> "SW 7400 4210". Empty if outside the Great Britain grid.
std::string ENToGridRef(EN const & en, int figures)
{
  if (en.easting < 0.0 || en.easting >= kMaxEasting || en.northing < 0.0 || en.northing >= kMaxNorthing)
    return {};

  int const e100k = static_cast<int>(en.easting / 100000.0);
  int const n100k = static_cast<int>(en.northing / 100000.0);

  // 5x5 grid of 500 km squares, each split into 5x5 squares of 100 km; the letter 'I' is skipped.
  int l1 = (19 - n100k) - (19 - n100k) % 5 + (e100k + 10) / 5;
  int l2 = (19 - n100k) * 5 % 25 + e100k % 5;
  if (l1 > 7)
    ++l1;
  if (l2 > 7)
    ++l2;

  long const divisor = math::PowUint(10L, static_cast<uint64_t>(5 - figures));
  long const eWithin = static_cast<long>(en.easting) % 100000 / divisor;
  long const nWithin = static_cast<long>(en.northing) % 100000 / divisor;

  std::string res;
  res += static_cast<char>('A' + l1);
  res += static_cast<char>('A' + l2);
  res += ' ';
  res += strings::to_string_width(eWithin, figures);
  res += ' ';
  res += strings::to_string_width(nWithin, figures);
  return res;
}

// "SW 740 421" / "SW740421" -> easting/northing. Nullopt if malformed or outside the grid.
std::optional<EN> GridRefToEN(std::string_view gridRef)
{
  std::string s;
  s.reserve(gridRef.size());
  for (char c : gridRef)
  {
    if (c == ' ' || c == '\t')
      continue;
    if (c >= 'a' && c <= 'z')
      c -= 'a' - 'A';
    s += c;
  }

  if (s.size() < 2)
    return {};

  char const c1 = s[0];
  char const c2 = s[1];
  if (c1 < 'A' || c1 > 'Z' || c1 == 'I' || c2 < 'A' || c2 > 'Z' || c2 == 'I')
    return {};

  int l1 = c1 - 'A';
  int l2 = c2 - 'A';
  if (l1 > 7)  // Compensate for the skipped 'I'.
    --l1;
  if (l2 > 7)
    --l2;

  int const e100k = (l1 - 2) % 5 * 5 + l2 % 5;
  int const n100k = 19 - l1 / 5 * 5 - l2 / 5;
  if (e100k < 0 || e100k * 100000 >= kMaxEasting || n100k < 0 || n100k * 100000 >= kMaxNorthing)
    return {};

  std::string_view digits(s);
  digits.remove_prefix(2);
  if (digits.empty() || digits.size() % 2 != 0 || digits.size() > 10)
    return {};

  size_t const figures = digits.size() / 2;
  long e = 0, nn = 0;
  for (size_t i = 0; i < figures; ++i)
  {
    if (digits[i] < '0' || digits[i] > '9' || digits[figures + i] < '0' || digits[figures + i] > '9')
      return {};
    e = e * 10 + (digits[i] - '0');
    nn = nn * 10 + (digits[figures + i] - '0');
  }

  // Scale the partial reference up to metres within the 100 km square (refers to its SW corner).
  long const multiplier = math::PowUint(10L, static_cast<uint64_t>(5 - figures));
  return EN{static_cast<double>(e100k * 100000 + e * multiplier),
            static_cast<double>(n100k * 100000 + nn * multiplier)};
}
}  // namespace

std::string FormatOSGrid(double lat, double lon, int figures)
{
  figures = math::Clamp(figures, 1, 5);

  // Cheap pre-filter: this lat/lon box contains the whole National Grid rectangle, so it only skips
  // points far outside it and never rejects one inside. The authoritative gate is the E/N range check
  // in ENToGridRef, which spans the rectangle (incl. Northern Ireland, the Republic of Ireland and the
  // surrounding sea); restricting the format to Great Britain by region is the caller's job (IsOSGridRegion).
  if (lat < 49.0 || lat > 61.5 || lon < -9.0 || lon > 2.5)
    return {};

  return ENToGridRef(Osgb36ToEN(Wgs84ToOsgb36({lat, lon})), figures);
}

std::optional<ms::LatLon> OSGridToLatLon(std::string_view gridRef)
{
  auto const en = GridRefToEN(gridRef);
  if (!en)
    return {};

  return Osgb36ToWgs84(ENToOsgb36(*en));
}

bool IsOSGridRegion(std::string_view regionId)
{
  // Organic Maps mwm region ids: "UK_England_*", "UK_Scotland*", "UK_Wales" and the separate
  // "Isle of Man". Matching these explicitly (rather than a bare "UK_" prefix) keeps out
  // "UK_Northern Ireland"; the Republic of Ireland is "Ireland", which has no "UK_" prefix.
  return regionId.starts_with("UK_England") || regionId.starts_with("UK_Scotland") ||
         regionId.starts_with("UK_Wales") || regionId == "Isle of Man";
}
}  // namespace os_grid_utils
