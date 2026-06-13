#include "platform/irish_grid_utils.hpp"

#include "geometry/geodesy.hpp"

#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <cmath>
#include <optional>
#include <string>
#include <string_view>

namespace irish_grid_utils
{
using math::DegToRad;

namespace
{
// Airy Modified 1849 (the Ireland 1965 / TM65 ellipsoid behind the Irish Grid). ITM is on ETRS89,
// which equals WGS84 to sub-millimetre here, so it reuses geodesy::kWgs84.
geodesy::Ellipsoid constexpr kAiryModified{6377340.189, 6356034.447};

// Irish Grid (TM65) and Irish Transverse Mercator (ITM, ETRS89). Same origin, different scale/ellipsoid.
geodesy::TMParams constexpr kIrishGrid{kAiryModified, 1.000035, DegToRad(53.5), DegToRad(-8.0), 200000.0, 250000.0};
geodesy::TMParams constexpr kITM{geodesy::kWgs84, 0.99982, DegToRad(53.5), DegToRad(-8.0), 600000.0, 750000.0};

// Irish Grid lettered area: a 5x5 array of 100 km squares, i.e. [0, 500 km) on each axis.
double constexpr kIrishGridMax = 500000.0;

// Coarse bounding box around the island of Ireland. It is a generous rectangle, NOT a true island
// test: its NE corner also reaches western Scotland (Kintyre, Arran, Islay) and it spans the
// surrounding sea. It is only a cheap pre-filter for the formatters; the authoritative "is this format
// official here" decision is the region check (IsIrishGridRegion), applied by the place page for
// display and by the search processor for the region-free parsers.
bool InIrelandBBox(ms::LatLon const & ll)
{
  return ll.m_lat >= 51.0 && ll.m_lat <= 55.7 && ll.m_lon >= -11.0 && ll.m_lon <= -5.0;
}

// WGS84 -> Ireland 1965 (TM65). Source: OSi/OSNI "Making maps compatible with GPS" (accurate to ~1-2 m).
geodesy::HelmertParams constexpr kWgs84ToTm65{
    -482.530, 130.596, -564.557, geodesy::SecToRad(-1.042), geodesy::SecToRad(-0.214), geodesy::SecToRad(-0.631),
    -8.150e-6};
geodesy::HelmertParams constexpr kTm65ToWgs84 = geodesy::InverseHelmert(kWgs84ToTm65);

ms::LatLon Wgs84ToTm65(ms::LatLon const & wgs)
{
  return geodesy::ShiftDatum(wgs, geodesy::kWgs84, kWgs84ToTm65, kAiryModified);
}

ms::LatLon Tm65ToWgs84(ms::LatLon const & tm65)
{
  return geodesy::ShiftDatum(tm65, kAiryModified, kTm65ToWgs84, geodesy::kWgs84);
}

// easting/northing -> "O 1516 3447". Empty outside the Irish Grid lettered area.
std::string ENToIrishGridRef(geodesy::EN const & en, int figures)
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
std::optional<geodesy::EN> IrishGridRefToEN(std::string_view gridRef)
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
  return geodesy::EN{static_cast<double>(e100k * 100000 + e * multiplier),
                     static_cast<double>(n100k * 100000 + nn * multiplier)};
}
}  // namespace

std::string FormatIrishGrid(double lat, double lon, int figures)
{
  figures = math::Clamp(figures, 1, 5);
  if (!InIrelandBBox({lat, lon}))
    return {};

  return ENToIrishGridRef(geodesy::LatLonToEN(Wgs84ToTm65({lat, lon}), kIrishGrid), figures);
}

std::optional<ms::LatLon> IrishGridToLatLon(std::string_view gridRef)
{
  auto const en = IrishGridRefToEN(gridRef);
  if (!en)
    return {};

  // Pure parse: a successful result only means a well-formed in-grid reference, not necessarily an
  // on-land Irish point (the lettered grid extends into the sea and into western Scotland). Callers
  // that need a real Irish location gate on the region (search does; see IsIrishGridRegion).
  return Tm65ToWgs84(geodesy::ENToLatLon(*en, kIrishGrid));
}

std::string FormatITM(double lat, double lon)
{
  if (!InIrelandBBox({lat, lon}))
    return {};

  // ETRS89 == WGS84 for our purposes, so no datum shift: project the WGS84 point directly.
  geodesy::EN const en = geodesy::LatLonToEN({lat, lon}, kITM);
  long const e = std::lround(en.easting);
  long const n = std::lround(en.northing);
  if (e < 0 || n < 0)
    return {};

  return std::to_string(e) + ' ' + std::to_string(n);
}

std::optional<ms::LatLon> ITMToLatLon(std::string_view itm)
{
  // Parse exactly two non-negative integers separated by whitespace and/or a single comma. Each group
  // is bounded to 7 digits (ITM eastings/northings are 6) so the accumulation cannot overflow a 32-bit
  // long for malformed input - the sibling grid parsers cap their digit counts for the same reason.
  long values[2] = {0, 0};
  int lengths[2] = {0, 0};
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
      if (++lengths[count - 1] > 7)
        return {};
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

  // Pure parse (see IrishGridToLatLon): geographic validity is the caller's concern.
  return geodesy::ENToLatLon(geodesy::EN{static_cast<double>(values[0]), static_cast<double>(values[1])}, kITM);
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
