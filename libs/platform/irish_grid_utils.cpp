#include "platform/irish_grid_utils.hpp"

#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <cmath>
#include <string>
#include <string_view>

namespace irish_grid_utils
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
  double constexpr ThirdFlattening() const { return (a - b) / (a + b); }
};

// WGS84 (also used for ITM: ETRS89/GRS80 equals WGS84 to sub-millimetre here) and Airy Modified 1849
// (the Ireland 1965 / TM65 ellipsoid behind the Irish Grid).
Ellipsoid constexpr kWgs84{6378137.0, 6356752.314245};
Ellipsoid constexpr kAiryModified{6377340.189, 6356034.447};

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

// Irish Grid (TM65) and Irish Transverse Mercator (ITM, ETRS89). Same origin, different scale/ellipsoid.
TMParams constexpr kIrishGrid{kAiryModified, 1.000035, DegToRad(53.5), DegToRad(-8.0), 200000.0, 250000.0};
TMParams constexpr kITM{kWgs84, 0.99982, DegToRad(53.5), DegToRad(-8.0), 600000.0, 750000.0};

// Irish Grid lettered area: a 5x5 array of 100 km squares, i.e. [0, 500 km) on each axis.
double constexpr kIrishGridMax = 500000.0;

// Generous bounding box of the island of Ireland. Used as a cheap pre-filter for the formatters and,
// for the region-free search parsers, as the anti-collision gate that an input must project back into.
bool InIreland(ms::LatLon const & ll)
{
  return ll.m_lat >= 51.0 && ll.m_lat <= 55.7 && ll.m_lon >= -11.0 && ll.m_lon <= -5.0;
}

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

// WGS84 -> Ireland 1965 (TM65). Source: OSi/OSNI "Making maps compatible with GPS" (accurate to ~1-2 m).
HelmertParams constexpr kWgs84ToTm65{-482.530,         130.596,          -564.557, SecToRad(-1.042),
                                     SecToRad(-0.214), SecToRad(-0.631), -8.150e-6};
// TM65 -> WGS84 is the inverse. Negating the linearized parameters is the standard approximation
// (round-trip error < 1 cm, far below the ~2 m Helmert accuracy).
HelmertParams constexpr kTm65ToWgs84 = InverseHelmert(kWgs84ToTm65);

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

ms::LatLon Wgs84ToTm65(ms::LatLon const & wgs)
{
  return CartesianToGeodetic(ApplyHelmert(GeodeticToCartesian(wgs, kWgs84), kWgs84ToTm65), kAiryModified);
}

ms::LatLon Tm65ToWgs84(ms::LatLon const & tm65)
{
  return CartesianToGeodetic(ApplyHelmert(GeodeticToCartesian(tm65, kAiryModified), kTm65ToWgs84), kWgs84);
}

struct EN
{
  double easting, northing;
};

// Meridional arc from the true origin to latitude phi for the given projection. Used by both directions.
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

// Radii of curvature (x scale factor) and eta^2 at latitude phi. Shared by both projection directions.
struct Curvature
{
  double nu;    // Transverse radius of curvature.
  double rho;   // Meridional radius of curvature.
  double eta2;  // nu / rho - 1.
};

Curvature ComputeCurvature(double sinPhi, TMParams const & tm)
{
  double const eSq = tm.ellipsoid.EccentricitySq();
  double const t = 1.0 - eSq * sinPhi * sinPhi;
  double const sqrtT = std::sqrt(t);
  double const nu = tm.ellipsoid.a * tm.F0 / sqrtT;
  double const rho = tm.ellipsoid.a * tm.F0 * (1.0 - eSq) / (t * sqrtT);
  return {nu, rho, nu / rho - 1.0};
}

// lat/lon (on the projection's own datum) -> easting/northing (Lee-Redfearn transverse Mercator).
EN LatLonToEN(ms::LatLon const & ll, TMParams const & tm)
{
  double const phi = DegToRad(ll.m_lat);
  double const lambda = DegToRad(ll.m_lon);
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
ms::LatLon ENToLatLon(EN const & en, TMParams const & tm)
{
  // Iterate the latitude until the meridional arc matches the requested northing.
  double phi = tm.lat0;
  double m = 0.0;
  do
  {
    phi += (en.northing - tm.N0 - m) / (tm.ellipsoid.a * tm.F0);
    m = MeridionalArc(phi, tm);
  }
  while (std::fabs(en.northing - tm.N0 - m) >= 0.00001);  // 0.01 mm.

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
  return {RadToDeg(lat), RadToDeg(lon)};
}

// easting/northing -> "O 1516 3447". Empty outside the Irish Grid lettered area.
std::string ENToIrishGridRef(EN const & en, int figures)
{
  if (en.easting < 0.0 || en.easting >= kIrishGridMax || en.northing < 0.0 || en.northing >= kIrishGridMax)
    return {};

  int const e100k = static_cast<int>(en.easting / 100000.0);   // 0..4
  int const n100k = static_cast<int>(en.northing / 100000.0);  // 0..4

  // Single-letter 5x5 grid, 'A' at the NW corner, row-major from the top; the letter 'I' is skipped.
  int letter = (4 - n100k) * 5 + e100k;  // 0..24
  if (letter >= 8)                       // 'I' is the 9th letter (index 8).
    ++letter;

  long const divisor = math::PowUint(10L, static_cast<uint64_t>(5 - figures));
  long const eWithin = static_cast<long>(en.easting) % 100000 / divisor;
  long const nWithin = static_cast<long>(en.northing) % 100000 / divisor;

  std::string res;
  res += static_cast<char>('A' + letter);
  res += ' ';
  res += strings::to_string_width(eWithin, figures);
  res += ' ';
  res += strings::to_string_width(nWithin, figures);
  return res;
}

// "O 152 345" / "O152345" -> easting/northing. Nullopt if malformed or outside the grid.
std::optional<EN> IrishGridRefToEN(std::string_view gridRef)
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

  if (s.empty())
    return {};

  char const c = s[0];
  if (c < 'A' || c > 'Z' || c == 'I')
    return {};

  int letter = c - 'A';
  if (c > 'I')  // Compensate for the skipped 'I'.
    --letter;
  int const e100k = letter % 5;
  int const n100k = 4 - letter / 5;

  std::string_view digits(s);
  digits.remove_prefix(1);
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

std::string FormatIrishGrid(double lat, double lon, int figures)
{
  figures = math::Clamp(figures, 1, 5);
  if (!InIreland({lat, lon}))
    return {};

  return ENToIrishGridRef(LatLonToEN(Wgs84ToTm65({lat, lon}), kIrishGrid), figures);
}

std::optional<ms::LatLon> IrishGridToLatLon(std::string_view gridRef)
{
  auto const en = IrishGridRefToEN(gridRef);
  if (!en)
    return {};

  ms::LatLon const ll = Tm65ToWgs84(ENToLatLon(*en, kIrishGrid));
  // The lettered grid extends into the sea around Ireland; reject references off the island so a
  // successful parse always yields a real on-island point.
  if (!InIreland(ll))
    return {};
  return ll;
}

std::string FormatITM(double lat, double lon)
{
  if (!InIreland({lat, lon}))
    return {};

  // ETRS89 == WGS84 for our purposes, so no datum shift: project the WGS84 point directly.
  EN const en = LatLonToEN({lat, lon}, kITM);
  long const e = std::lround(en.easting);
  long const n = std::lround(en.northing);
  if (e < 0 || n < 0)
    return {};

  return std::to_string(e) + ' ' + std::to_string(n);
}

std::optional<ms::LatLon> ITMToLatLon(std::string_view itm)
{
  // Parse exactly two non-negative integers separated by whitespace and/or a single comma.
  long values[2] = {0, 0};
  int count = 0;
  bool inNumber = false;
  for (char const c : itm)
  {
    if (c >= '0' && c <= '9')
    {
      if (!inNumber)
      {
        if (count == 2)
          return {};
        values[count] = 0;
        ++count;
        inNumber = true;
      }
      values[count - 1] = values[count - 1] * 10 + (c - '0');
    }
    else if (c == ' ' || c == '\t' || c == ',')
    {
      inNumber = false;
    }
    else
    {
      return {};
    }
  }

  if (count != 2)
    return {};

  ms::LatLon const ll = ENToLatLon(EN{static_cast<double>(values[0]), static_cast<double>(values[1])}, kITM);
  if (!InIreland(ll))
    return {};
  return ll;
}

bool IsIrishGridRegion(std::string_view regionId)
{
  // Both jurisdictions of the island use the Irish systems. The Republic of Ireland is split into
  // "Ireland_<province>" leaf mwms (e.g. "Ireland_Leinster", "Ireland_Munster"), and GetRegionCountryId
  // returns the leaf, so match it by prefix; Northern Ireland is the single "UK_Northern Ireland" mwm.
  // IsOSGridRegion excludes both, so the OS Grid and the Irish systems are never offered at one point.
  return regionId == "UK_Northern Ireland" || regionId.starts_with("Ireland");
}
}  // namespace irish_grid_utils
