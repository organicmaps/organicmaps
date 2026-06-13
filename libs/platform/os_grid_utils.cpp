#include "platform/os_grid_utils.hpp"

#include "geometry/geodesy.hpp"

#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace os_grid_utils
{
using math::DegToRad;

namespace
{
geodesy::Ellipsoid constexpr kAiry1830{6377563.396, 6356256.909};

// National Grid transverse Mercator definition (OSGB36 / Airy 1830): origin 49°N 2°W, false origin
// (400 km E, -100 km N), central-meridian scale 0.9996012717.
geodesy::TMParams constexpr kNationalGrid{kAiry1830, 0.9996012717, DegToRad(49.0), DegToRad(-2.0), 400000.0, -100000.0};

// National Grid extent, metres: a valid two-letter square exists only within [0, 700 km) x [0, 1300 km).
double constexpr kMaxEasting = 700000.0;
double constexpr kMaxNorthing = 1300000.0;

// WGS84 -> OSGB36. Source: Ordnance Survey "A guide to coordinate systems in Great Britain".
geodesy::HelmertParams constexpr kWgs84ToOsgb36{
    -446.448,  125.157, -542.060, geodesy::SecToRad(-0.1502), geodesy::SecToRad(-0.2470), geodesy::SecToRad(-0.8421),
    20.4894e-6};
geodesy::HelmertParams constexpr kOsgb36ToWgs84 = geodesy::InverseHelmert(kWgs84ToOsgb36);

// easting/northing -> "SW 7400 4210". Empty if outside the Great Britain grid.
std::string ENToGridRef(geodesy::EN const & en, int figures)
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
std::optional<geodesy::EN> GridRefToEN(std::string_view gridRef)
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
  return geodesy::EN{static_cast<double>(e100k * 100000 + e * multiplier),
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

  auto const osgb = geodesy::ShiftDatum({lat, lon}, geodesy::kWgs84, kWgs84ToOsgb36, kAiry1830);
  return ENToGridRef(geodesy::LatLonToEN(osgb, kNationalGrid), figures);
}

std::optional<ms::LatLon> OSGridToLatLon(std::string_view gridRef)
{
  auto const en = GridRefToEN(gridRef);
  if (!en)
    return {};

  return geodesy::ShiftDatum(geodesy::ENToLatLon(*en, kNationalGrid), kAiry1830, kOsgb36ToWgs84, geodesy::kWgs84);
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
