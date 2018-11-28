#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/math.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>

using namespace settings;
using namespace std;
using namespace strings;

namespace measurement_utils
{
string ToStringPrecision(double d, int pr)
{
  stringstream ss;
  ss << setprecision(pr) << fixed << d;
  return ss.str();
}

bool FormatDistanceImpl(double m, string & res,
                        char const * high, char const * low,
                        double highF, double lowF)
{
  double const lowV = m / lowF;
  if (lowV < 1.0)
  {
    res = string("0") + low;
    return false;
  }

  // To display any lower units only if < 1000
  if (m >= 1000.0 * lowF)
  {
    double const v = m / highF;
    res = ToStringPrecision(v, v >= 10.0 ? 0 : 1) + high;
  }
  else
  {
    // To display unit number only if <= 100.
    res = ToStringPrecision(lowV <= 100.0 ? lowV : round(lowV / 10) * 10, 0) + low;
  }

  return true;
}

double ToSpeedKmPH(double speed, measurement_utils::Units units)
{
  switch (units)
  {
  case Units::Imperial: return MphToKmph(speed);
  case Units::Metric: return speed;
  }
  UNREACHABLE();
}

bool FormatDistance(double m, string & res)
{
  auto units = Units::Metric;
  TryGet(settings::kMeasurementUnits, units);

  /// @todo Put string units resources.
  switch (units)
  {
  case Units::Imperial: return FormatDistanceImpl(m, res, " mi", " ft", 1609.344, 0.3048);
  case Units::Metric: return FormatDistanceImpl(m, res, " km", " m", 1000.0, 1.0);
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
    d = rounds(d);

  d = modf(d, &i);
  sstream << setw(2) << i;

  if (dac > 0)
    sstream << to_string_dac(d, dac).substr(1);

  sstream << "″";

  // This condition is too heavy for production purposes (but more correct).
  //if (base::rounds(value * 3600.0 * pow(10, dac)) != 0)
  if (!AlmostEqualULPs(value, 0.0))
  {
    char postfix = positive;
    if (value < 0.0)
      postfix = negative;

    sstream << postfix;
  }

  return sstream.str();
}

string FormatLatLonAsDMS(double lat, double lon, int dac)
{
  return (FormatLatLonAsDMSImpl(lat, 'N', 'S', dac) + " "  +
          FormatLatLonAsDMSImpl(lon, 'E', 'W', dac));
}

void FormatLatLonAsDMS(double lat, double lon, string & latText, string & lonText, int dac)
{
  latText = FormatLatLonAsDMSImpl(lat, 'N', 'S', dac);
  lonText = FormatLatLonAsDMSImpl(lon, 'E', 'W', dac);
}

void FormatMercatorAsDMS(m2::PointD const & mercator, string & lat, string & lon, int dac)
{
  lat = FormatLatLonAsDMSImpl(MercatorBounds::YToLat(mercator.y), 'N', 'S', dac);
  lon = FormatLatLonAsDMSImpl(MercatorBounds::XToLon(mercator.x), 'E', 'W', dac);
}

string FormatMercatorAsDMS(m2::PointD const & mercator, int dac)
{
  return FormatLatLonAsDMS(MercatorBounds::YToLat(mercator.y), MercatorBounds::XToLon(mercator.x), dac);
}

// @TODO take into account decimal points or commas as separators in different locales
string FormatLatLon(double lat, double lon, int dac)
{
  return to_string_dac(lat, dac) + " " + to_string_dac(lon, dac);
}

string FormatLatLon(double lat, double lon, bool withSemicolon, int dac)
{
  return to_string_dac(lat, dac) + (withSemicolon ? ", " : " ") + to_string_dac(lon, dac);
}

void FormatLatLon(double lat, double lon, string & latText, string & lonText, int dac)
{
  latText = to_string_dac(lat, dac);
  lonText = to_string_dac(lon, dac);
}

string FormatMercator(m2::PointD const & mercator, int dac)
{
  return FormatLatLon(MercatorBounds::YToLat(mercator.y), MercatorBounds::XToLon(mercator.x), dac);
}

void FormatMercator(m2::PointD const & mercator, string & lat, string & lon, int dac)
{
  lat = to_string_dac(MercatorBounds::YToLat(mercator.y), dac);
  lon = to_string_dac(MercatorBounds::XToLon(mercator.x), dac);
}

string FormatAltitude(double altitudeInMeters)
{
  Units units = Units::Metric;
  TryGet(settings::kMeasurementUnits, units);

  ostringstream ss;
  ss << fixed << setprecision(0);

  /// @todo Put string units resources.
  switch (units)
  {
  case Units::Imperial: ss << MetersToFeet(altitudeInMeters) << "ft"; break;
  case Units::Metric: ss << altitudeInMeters << "m"; break;
  }
  return ss.str();
}

string FormatSpeedWithDeviceUnits(double metersPerSecond)
{
  auto units = Units::Metric;
  TryGet(settings::kMeasurementUnits, units);
  return FormatSpeedWithUnits(metersPerSecond, units);
}

string FormatSpeedWithUnits(double metersPerSecond, Units units)
{
  return FormatSpeed(metersPerSecond, units) + FormatSpeedUnits(units);
}

string FormatSpeed(double metersPerSecond, Units units)
{
  double constexpr kSecondsPerHour = 3600;
  double constexpr metersPerKilometer = 1000;
  double unitsPerHour;
  switch (units)
  {
  case Units::Imperial: unitsPerHour = MetersToMiles(metersPerSecond) * kSecondsPerHour; break;
  case Units::Metric: unitsPerHour = metersPerSecond * kSecondsPerHour / metersPerKilometer; break;
  }
  return ToStringPrecision(unitsPerHour, unitsPerHour >= 10.0 ? 0 : 1);
}

string FormatSpeedUnits(Units units)
{
  switch (units)
  {
  case Units::Imperial: return "mph";
  case Units::Metric: return "km/h";
  }
  UNREACHABLE();
}

bool OSMDistanceToMeters(string const & osmRawValue, double & outMeters)
{
  char * stop;
  char const * s = osmRawValue.c_str();
  outMeters = strtod(s, &stop);

  // Not a number, was not parsed at all.
  if (s == stop)
    return false;

  if (!isfinite(outMeters))
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
      if (s != stop && *stop == '"' && isfinite(inches))
        outMeters += InchesToMeters(inches);
      return true;
    }
    break;

  // Inches.
  case '\"': outMeters = InchesToMeters(outMeters); return true;

  // It's probably a range. Use maximum value (if possible) for a range.
  case '-':
    {
      s = stop + 1;
      double const newValue = strtod(s, &stop);
      if (s != stop && isfinite(newValue))
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
  else if (strstr(stop, "ft") == stop)
    outMeters = FeetToMeters(outMeters);
  else if (strstr(stop, "feet") == stop)
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
