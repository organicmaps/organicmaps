#include "measurement_utils.hpp"

#include "../platform/settings.hpp"

#include "../indexer/mercator.hpp"

#include "../base/string_utils.hpp"
#include "../base/math.hpp"

#include "../std/iomanip.hpp"
#include "../std/sstream.hpp"


namespace MeasurementUtils
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

  if (m >= highF)
  {
    double const v = m / highF;
    res = ToStringPrecision(v, v >= 10.0 ? 0 : 1) + high;
  }
  else
    res = ToStringPrecision(lowV, 0) + low;

  return true;
}

bool FormatDistance(double m, string & res)
{
  using namespace Settings;
  Units u = Metric;
  Settings::Get("Units", u);

  /// @todo Put string units resources.
  switch (u)
  {
  case Yard: return FormatDistanceImpl(m, res, " mi", " yd", 1609.344, 0.9144);
  case Foot: return FormatDistanceImpl(m, res, " mi", " ft", 1609.344, 0.3048);
  default: return FormatDistanceImpl(m, res, " km", " m", 1000.0, 1.0);
  }
}


string FormatLatLonAsDMSImpl(double value, char positive, char negative, int dac)
{
  using namespace my;

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
    sstream << strings::to_string_dac(d, dac).substr(1);

  sstream << "″";

  // This condition is too heavy for production purposes (but more correct).
  //if (my::rounds(value * 3600.0 * pow(10, dac)) != 0)
  if (!AlmostEqual(value, 0.0))
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
  return strings::to_string_dac(lat, dac) + " " + strings::to_string_dac(lon, dac);
}

string FormatMercator(m2::PointD const & mercator, int dac)
{
  return FormatLatLon(MercatorBounds::YToLat(mercator.y), MercatorBounds::XToLon(mercator.x), dac);
}

void FormatMercator(m2::PointD const & mercator, string & lat, string & lon, int dac)
{
  lat = FormatLatLon(MercatorBounds::YToLat(mercator.y), dac);
  lon = FormatLatLon(MercatorBounds::XToLon(mercator.x), dac);
}

string FormatAltitude(double altitudeInMeters)
{
  using namespace Settings;
  Units u = Metric;
  (void)Settings::Get("Units", u);

  ostringstream sstream;
  sstream << std::fixed << setprecision(0);
  /// @todo Put string units resources.
  switch (u)
  {
  case Yard: sstream << MetersToYards(altitudeInMeters) << "yd"; break;
  case Foot: sstream << MetersToFeet(altitudeInMeters) << "ft"; break;
  default: sstream << altitudeInMeters << "m"; break;
  }
  return sstream.str();
}

string FormatSpeed(double metersPerSecond)
{
  using namespace Settings;
  Units u = Metric;
  (void)Settings::Get("Units", u);

  double perHour;
  string res;

  /// @todo Put string units resources.
  switch (u)
  {
  case Yard:
  case Foot:
    perHour = metersPerSecond * 3600. / 1609.344;
    res = ToStringPrecision(perHour, perHour >= 10.0 ? 0 : 1) + "mph";
    break;
  default:
    perHour = metersPerSecond * 3600. / 1000.;
    res = ToStringPrecision(perHour, perHour >= 10.0 ? 0 : 1) + "km/h";
    break;
  }
  return res;
}

} // namespace MeasurementUtils
