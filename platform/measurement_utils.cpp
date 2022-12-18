#include "platform/locale.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/macros.hpp"
#include "base/math.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <cmath>
#include <cstring>    // strstr
#include <iomanip>
#include <sstream>

namespace measurement_utils
{
using namespace platform;
using namespace settings;
using namespace std;
using namespace strings;

string ToStringPrecision(double d, int pr)
{
  // We assume that the app will be restarted if a user changes device's locale.
  static Locale const loc = GetCurrentLocale();

  return ToStringPrecisionLocale(loc, d, pr);
}

string ToStringPrecisionLocale(Locale loc, double d, int pr)
{
  stringstream ss;
  ss << setprecision(pr) << fixed << d;
  string out = ss.str();

  // std::locale does not work on Android NDK, so decimal and grouping (thousands) separator
  // shall be customized manually here.

  if (pr)
  {
    // Value with decimals. Set locale decimal separator.
    if (loc.m_decimalSeparator != ".")
      out.replace(out.size() - pr - 1, 1, loc.m_decimalSeparator);
  }
  else
  {
    // Value with no decimals. Check if it's equal or bigger than 10000 to
    // insert the grouping (thousands) separator characters.
    if (out.size() > 4 && !loc.m_groupingSeparator.empty())
      for (int pos = out.size() - 3; pos > 0; pos -= 3)
        out.insert(pos, loc.m_groupingSeparator);
  }

  return out;
}

std::string DebugPrint(Units units)
{
  switch (units)
  {
  case Units::Imperial: return "Units::Imperial";
  case Units::Metric: return "Units::Metric";
  }
  UNREACHABLE();
}

Units GetMeasurementUnits()
{
  Units units = measurement_utils::Units::Metric;
  settings::TryGet(settings::kMeasurementUnits, units);
  return units;
}

double ToSpeedKmPH(double speed, Units units)
{
  switch (units)
  {
  case Units::Imperial: return MiphToKmph(speed);
  case Units::Metric: return speed;
  }
  UNREACHABLE();
}

string FormatLatLonAsDMSImpl(double value, char positive, char negative, int dac)
{
  using namespace base;

  ostringstream sstream;
  sstream << setfill('0');

  // Degrees
  double i;
  double d = modf(fabs(value), &i);
  sstream << setw(2) << i << "°";

  // Minutes
  d = modf(d * 60.0, &i);
  sstream << setw(2) << i << "′";

  // Seconds
  d = d * 60.0;
  if (dac == 0)
    d = SignedRound(d);

  d = modf(d, &i);
  sstream << setw(2) << i;

  if (dac > 0)
    sstream << to_string_dac(d, dac).substr(1);

  sstream << "″";

  // This condition is too heavy for production purposes (but more correct).
  //if (base::SignedRound(value * 3600.0 * pow(10, dac)) != 0)
  if (!AlmostEqualULPs(value, 0.0))
  {
    char postfix = positive;
    if (value < 0.0)
      postfix = negative;

    sstream << postfix;
  }

  return sstream.str();
}

string FormatLatLonAsDMS(double lat, double lon, bool withComma, int dac)
{
  return (FormatLatLonAsDMSImpl(lat, 'N', 'S', dac) + (withComma ? ", " : " ") +
          FormatLatLonAsDMSImpl(lon, 'E', 'W', dac));
}

void FormatLatLonAsDMS(double lat, double lon, string & latText, string & lonText, int dac)
{
  latText = FormatLatLonAsDMSImpl(lat, 'N', 'S', dac);
  lonText = FormatLatLonAsDMSImpl(lon, 'E', 'W', dac);
}

void FormatMercatorAsDMS(m2::PointD const & mercator, string & lat, string & lon, int dac)
{
  lat = FormatLatLonAsDMSImpl(mercator::YToLat(mercator.y), 'N', 'S', dac);
  lon = FormatLatLonAsDMSImpl(mercator::XToLon(mercator.x), 'E', 'W', dac);
}

string FormatMercatorAsDMS(m2::PointD const & mercator, int dac)
{
  return FormatLatLonAsDMS(mercator::YToLat(mercator.y), mercator::XToLon(mercator.x), dac);
}

// @TODO take into account decimal points or commas as separators in different locales
string FormatLatLon(double lat, double lon, int dac)
{
  return to_string_dac(lat, dac) + " " + to_string_dac(lon, dac);
}

string FormatLatLon(double lat, double lon, bool withComma, int dac)
{
  return to_string_dac(lat, dac) + (withComma ? ", " : " ") + to_string_dac(lon, dac);
}

void FormatLatLon(double lat, double lon, string & latText, string & lonText, int dac)
{
  latText = to_string_dac(lat, dac);
  lonText = to_string_dac(lon, dac);
}

string FormatMercator(m2::PointD const & mercator, int dac)
{
  return FormatLatLon(mercator::YToLat(mercator.y), mercator::XToLon(mercator.x), dac);
}

void FormatMercator(m2::PointD const & mercator, string & lat, string & lon, int dac)
{
  lat = to_string_dac(mercator::YToLat(mercator.y), dac);
  lon = to_string_dac(mercator::XToLon(mercator.x), dac);
}

double MpsToUnits(double metersPerSecond, Units units)
{
  switch (units)
  {
  case Units::Imperial: return KmphToMiph(MpsToKmph(metersPerSecond)); break;
  case Units::Metric: return MpsToKmph(metersPerSecond); break;
  }
  UNREACHABLE();
}

string FormatSpeedNumeric(double metersPerSecond, Units units)
{
  double const unitsPerHour = MpsToUnits(metersPerSecond, units);
  return ToStringPrecision(unitsPerHour, unitsPerHour >= 10.0 ? 0 : 1);
}

string FormatOsmLink(double lat, double lon, int zoom)
{
  static constexpr char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_~";

  // Same as (lon + 180) / 360 * 1UL << 32, but without warnings.
  double constexpr factor = (1 << 30) / 90.0;
  uint32_t const x = round((lon + 180.0) * factor);
  uint32_t const y = round((lat + 90.0) * factor * 2.0);
  uint64_t const code = bits::BitwiseMerge(y, x);
  string osmUrl = "https://osm.org/go/";

  for (int i = 0; i < (zoom + 10) / 3; ++i)
  {
    const uint64_t digit = (code >> (58 - 6 * i)) & 0x3f;
    ASSERT_LESS(digit, ARRAY_SIZE(chars), ());
    osmUrl += chars[digit];
  }

  for (int i = 0; i < (zoom + 8) % 3; ++i)
    osmUrl += "-";

  return osmUrl + "?m=";
}

bool OSMDistanceToMeters(string const & osmRawValue, double & outMeters)
{
  using strings::is_finite;

  char * stop;
  char const * s = osmRawValue.c_str();
  outMeters = strtod(s, &stop);

  // Not a number, was not parsed at all.
  if (s == stop)
    return false;

  if (!is_finite(outMeters))
    return false;

  switch (*stop)
  {
  // Default units - meters.
  case 0: return true;

  // Feet and probably inches.
  case '\'':
    {
      outMeters = FeetToMeters(outMeters);
      s = stop + 1;
      double const inches = strtod(s, &stop);
      if (s != stop && *stop == '"' && is_finite(inches))
        outMeters += InchesToMeters(inches);
      return true;
    }
    break;

  // Inches.
  case '"': outMeters = InchesToMeters(outMeters); return true;

  // It's probably a range. Use maximum value (if possible) for a range.
  case '-':
    {
      s = stop + 1;
      double const newValue = strtod(s, &stop);
      if (s != stop && is_finite(newValue))
        outMeters = newValue;
    }
    break;

  // It's probably a list. We don't support them.
  case ';': return false;
  }

  while (*stop && isspace(*stop))
    ++stop;

  // Default units - meters.
  if (*stop == 0)
    return true;

  if (strstr(stop, "nmi") == stop)
    outMeters = NauticalMilesToMeters(outMeters);
  else if (strstr(stop, "mi") == stop)
    outMeters = MilesToMeters(outMeters);
  else if (strstr(stop, "ft") == stop || strstr(stop, "feet") == stop)
    outMeters = FeetToMeters(outMeters);
  else if (strstr(stop, "km") == stop)
    outMeters = outMeters * 1000;

  // Count all other cases as meters.
  return true;
}

string OSMDistanceToMetersString(string const & osmRawValue,
                                 bool supportZeroAndNegativeValues,
                                 int digitsAfterComma)
{
  double meters;
  if (OSMDistanceToMeters(osmRawValue, meters))
  {
    if (!supportZeroAndNegativeValues && meters <= 0)
      return {};
    return strings::to_string_dac(meters, digitsAfterComma);
  }
  return {};
}
}  // namespace measurement_utils
